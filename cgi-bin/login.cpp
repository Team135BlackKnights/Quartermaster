#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include "subsystem.h"
#include "home.h"
#include "parts.h"
#include <openssl/sha.h>

#include "login.h"
using namespace std;
string generate_token()
{
	static random_device rd;
	static mt19937 gen(rd());
	static uniform_int_distribution<> dis(0, 15);

	stringstream ss;
	for (int i = 0; i < 32; i++)
		ss << hex << dis(gen);
	return ss.str();
}

string hash_password(string const &pass)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256((unsigned char *)pass.c_str(), pass.size(), hash);

	stringstream ss;
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
		ss << hex << setw(2) << setfill('0') << (int)hash[i];
	return ss.str();
}

bool check_password(string input, string hash_from_db)
{
	return hash_password(input) == hash_from_db;
}

void show_login_form(ostream &o, const string &message = "")
{
	std::cout << "Content-Type: text/html\r\n\r\n";
	std::cout << "<html><body>";
	std::cout << "<h2>Login</h2>";

	if (!message.empty())
	{
		std::cout << "<p style='color:red;'>" << message << "</p>";
	}

	std::cout << R"(
		<form method="GET" action="/cgi-bin/parts.cgi">
			<input type="hidden" name="p" value="Login">
			<label>Username: <input name="username" required></label><br>
			<label>Password: <input type="password" name="password" required></label><br>
			<input type="submit" value="Login">
		</form>
	)";
	std::cout << "</body></html>";
}

// Utility function to escape user inputs (SQL injection prevention)
string escape(DB db, const string &input)
{
	char *escaped = new char[input.length() * 2 + 1];
	mysql_real_escape_string(db, escaped, input.c_str(), input.length());
	string result(escaped);
	delete[] escaped;
	return result;
}
#define DEBUG_OUTPUT(o) o << "Debug: " << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << endl;

// Main logic for handling login
void inner(std::ostream &o, Login const &a, DB db)
{
	try
	{
		// Ensure required tables exist
		query(db, R"(
			CREATE TABLE IF NOT EXISTS users (
				username VARCHAR(255) PRIMARY KEY,
				password_hash TEXT NOT NULL
			)
		)");
		query(db, R"(
			CREATE TABLE IF NOT EXISTS sessions (
				id INT AUTO_INCREMENT PRIMARY KEY,
				username VARCHAR(255),
				session_token TEXT,
				created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
			)
		)");

		string user = current_user(db);
		if (user != "no_user")
		{
			o << "<html><head><meta http-equiv='refresh' content='0; url=/cgi-bin/parts.cgi'></head>";
			o << "<body>Already logged in. Redirecting...</body></html>";
			return;
		}

		// Check user count
		auto user_count = query(db, "SELECT COUNT(*) as count FROM users");
		if (user_count.empty() || user_count[0].empty() || !user_count[0][0].has_value())
		{
			o << "<p>Error: Could not retrieve user count from database.</p>";
			return;
		}

		string count_str = *user_count[0][0];

		// Create default admin user if none exist
		if (count_str == "0")
		{
			string hashed = hash_password("admin");
			string username = "admin";
			query(db, "INSERT INTO users(username, password_hash) VALUES('" + escape(db, username) + "', '" + escape(db, hashed) + "')");
			o << "<p>First user '" << username << "' created. Please log in.</p>";
			show_login_form(o);
			return;
		}

		// Handle login form submission
		if (!a.username.empty() && !a.password.empty())
		{
			string q = "SELECT password_hash FROM users WHERE username = '" + escape(db, a.username) + "'";
			auto rows = query(db, q);

			if (rows.empty() || rows[0].empty() || !rows[0][0].has_value())
			{
				show_login_form(o, "Login failed. Try again.");
				return;
			}

			if (!check_password(a.password, *rows[0][0]))
			{
				show_login_form(o, "Incorrect password. Try again.");
				return;
			}

			// Generate session and token
			string token = generate_token();
			query(db, "INSERT INTO sessions(username, session_token) VALUES('" + escape(db, a.username) + "', '" + escape(db, token) + "')");

			// === VERY IMPORTANT ===
			// Print headers via std::cout before any body output
			std::cout << "Set-Cookie: session_token=" << token << "; Path=/; HttpOnly\r\n";
			std::cout << "Content-Type: text/html\r\n\r\n";

			// Print the HTML body to std::cout — not to `o`
			std::cout << "<html><head><meta http-equiv='refresh' content='0; url=/cgi-bin/parts.cgi'></head>";
			std::cout << "<body>Login successful. Redirecting...</body></html>";
			return;
		}

		// No data, show login form
		show_login_form(o);
	}
	catch (const std::exception &e)
	{
		o << "<p>Error: " << e.what() << "</p>";
	}
	catch (...)
	{
		o << "<p>Unknown error occurred.</p>";
	}
}
