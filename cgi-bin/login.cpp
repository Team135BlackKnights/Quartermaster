#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "subsystem.h"
#include "home.h"
#include "parts.h"
#include "login.h"
using namespace std;
string generate_token() {
	static random_device rd;
	static mt19937 gen(rd());
	static uniform_int_distribution<> dis(0, 15);

	stringstream ss;
	for (int i = 0; i < 32; i++)
		ss << hex << dis(gen);
	return ss.str();
}

string hash_password(string const& pass) {
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256((unsigned char*)pass.c_str(), pass.size(), hash);

	stringstream ss;
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
		ss << hex << setw(2) << setfill('0') << (int)hash[i];
	return ss.str();
}

bool check_password(string input, string hash_from_db) {
	return hash_password(input) == hash_from_db;
}

void login_page(ostream &o) {
	o << "Content-Type: text/html\r\n\r\n";
	o << R"(
		<h2>Login</h2>
		<form method="POST" action="/login.cgi">
			<label>Username: <input name="username"></label><br>
			<label>Password: <input type="password" name="password"></label><br>
			<input type="submit" value="Login">
		</form>
	)";
}

void inner(std::ostream &o, Login const &a, MYSQL* mysql)
{
	DB_connection con{DB{mysql}};

	if (a.username.empty() || a.password.empty()) {
		login_page(o);
		return;
	}

	// Fetch hashed password
	string query = "SELECT password_hash FROM users WHERE username = '" + escape(con, a.username) + "'";
	auto rows = con.query(query);
	if (rows.empty() || !check_password(a.password, rows[0]["password_hash"])) {
		o << "Content-Type: text/html\r\n\r\n";
		o << "Login failed. <a href='/login.cgi'>Try again</a>";
		return;
	}

	// Generate session
	string token = generate_token();
	con.query("INSERT INTO sessions(username, session_token) VALUES('" + escape(con, a.username) + "', '" + token + "')");

	o << "Set-Cookie: session_token=" << token << "; Path=/; HttpOnly\r\n";
	o << "Status: 302 Found\r\n";
	o << "Location: /\r\n\r\n";
}
