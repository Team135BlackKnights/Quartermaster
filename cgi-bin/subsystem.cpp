#include "subsystem.h"
#include "home.h"
#include "subsystems.h"
#include "part.h"
#include "meeting.h"
#include <ctime>
#include <map>
#include <cstdlib>
#include "auth.h"
#include "parts.h"
using namespace std;
inline std::string generate_session_id() {
	std::string charset = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::string result;
	for (int i = 0; i < 32; i++) {
		result += charset[rand() % charset.size()];
	}
	return result;
}

inline void save_session(MYSQL* db, const std::string& user, const std::string& session_id) {
	string q = "REPLACE INTO sessions (session_id, username) VALUES (?, ?)";
	auto stmt = mysql_stmt_init(db);
	mysql_stmt_prepare(stmt, q.c_str(), q.size());

	MYSQL_BIND bind[2] = {};
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (void*)session_id.c_str();
	bind[0].buffer_length = session_id.size();
	bind[1].buffer_type = MYSQL_TYPE_STRING;
	bind[1].buffer = (void*)user.c_str();
	bind[1].buffer_length = user.size();
	mysql_stmt_bind_param(stmt, bind);
	mysql_stmt_execute(stmt);
	mysql_stmt_close(stmt);
}

inline std::string get_user_by_session(MYSQL* db, const std::string& session_id) {
	string q = "SELECT username FROM sessions WHERE session_id = ?";
	auto stmt = mysql_stmt_init(db);
	mysql_stmt_prepare(stmt, q.c_str(), q.size());

	MYSQL_BIND bind[1] = {};
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (void*)session_id.c_str();
	bind[0].buffer_length = session_id.size();
	mysql_stmt_bind_param(stmt, bind);
	mysql_stmt_execute(stmt);

	char user[256] = {};
	MYSQL_BIND result[1] = {};
	result[0].buffer_type = MYSQL_TYPE_STRING;
	result[0].buffer = user;
	result[0].buffer_length = sizeof(user);
	mysql_stmt_bind_result(stmt, result);
	mysql_stmt_store_result(stmt);

	std::string result_user = "no_user";
	if (mysql_stmt_fetch(stmt) == 0) {
		result_user = user;
	}

	mysql_stmt_close(stmt);
	return result_user;
}

inline std::string get_cookie(const std::string& cookie_header, const std::string& key) {
	auto pos = cookie_header.find(key + "=");
	if (pos == std::string::npos) return "";
	auto end = cookie_header.find(";", pos);
	return cookie_header.substr(pos + key.size() + 1, end - pos - key.size() - 1);
}
map<string, string> parse_cookies() {
	map<string, string> cookies;
	const char* raw = getenv("HTTP_COOKIE");
	if (!raw) return cookies;

	string cookie_str(raw);
	stringstream ss(cookie_str);
	string pair;
	while (getline(ss, pair, ';')) {
		auto eq = pair.find('=');
		if (eq != string::npos) {
			string key = pair.substr(0, eq);
			string value = pair.substr(eq + 1);
			while (!key.empty() && key[0] == ' ') key.erase(0, 1); // trim left spaces
			cookies[key] = value;
		}
	}
	return cookies;
}
std::string current_user() {
	const char* raw_cookie = getenv("HTTP_COOKIE");
	if (!raw_cookie) return "no_user";

	std::string session_token;
	auto cookies = parse_cookies();
	auto it = cookies.find("session_token");
	if (it != cookies.end()) {
		session_token = it->second;
	}
	if (session_token.empty()) return "no_user";

	// Setup DB connection
	DB db = mysql_init(NULL);
	if (!db) return "no_user";

	auto creds = auth();
	if (!mysql_real_connect(db, creds.host.c_str(), creds.user.c_str(), creds.pass.c_str(), creds.db.c_str(), 0, NULL, 0)) {
		mysql_close(db);
		return "no_user";
	}

	// Escape session token
	std::string query = "SELECT username FROM sessions WHERE session_id = '" + session_token + "'";
	//Create Con
	DB_connection con{db};
	auto rows = con.query(query);
	if (rows.empty()) return "no_user";

	return rows[0]["username"];
}

Id new_item(DB db,Table_name const& table){
	run_cmd(db,"INSERT INTO "+table+" VALUES ()");
	auto q=query(db,"SELECT LAST_INSERT_ID()");
	//PRINT(q);
	assert(q.size()==1);
	assert(q[0].size()==1);
	assert(q[0][0]);
	auto id=stoi(*q[0][0]);
	return id;
}

void inner(ostream& o,Subsystem_new const& a,DB db){
	string user = current_user();
	if (user == "no_user") {
		cout << "Location: /cgi-bin/login.cgi\n\n";
	}
	vector<string> info_col_names;
	#define X(A,B) info_col_names|=""#B;
	SUBSYSTEM_INFO_ROW(X)
	#undef X

	Subsystem_data current{};
	current.valid=1;
	if(a.parent){
		current.parent=a.parent;
	}
	make_page(
		o,
		[&]()->string{
			if(a.parent){
				return "New child subsystem of "+subsystem_name(db,*a.parent);
			}
			return "New Subsystem";
		}(),
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\"Subsystem_new_data\">"
		//"<input type=\"hidden\" name=\"subsystem_id\" value=\""+escape(a.id)+"\">"+
		//+after_done()
		+input_table([=](){
			vector<Input> r;
			#define X(A,B) r|=show_input(db,""#B,current.B);
			SUBSYSTEM_DATA(X)
			#undef X
			return r;
		}())+
		"<br><input type=\"submit\">"+
		"</form>"
	);
	/*auto id=new_item(db,"subsystem");

	if(a.parent){
		auto q="INSERT INTO subsystem_info ("
			"subsystem_id,"
			"edit_user,"
			"edit_date,"
			"valid,"
			"parent "
			") VALUES ("
			+escape(id)+","
			+escape(current_user())+","
			"now(),"
			"1,"
			+escape(a.parent)
			+")";
		run_cmd(db,q);
	}
	Subsystem_editor page;
	page.id={id};
	make_page(
		o,
		"New subsystem",
		redirect_to(page)
	);*/
}

void inner(ostream& o,Subsystem_new_data const& a,DB db){
	string user = current_user();
	if (user == "no_user") {
		cout << "Location: /cgi-bin/login.cgi\n\n";
	}
	auto id=Subsystem_id{new_item(db,"subsystem")};
	if(a.parent){
		auto q="INSERT INTO subsystem_info ("
			"subsystem_id,"
			"edit_user,"
			"edit_date,"
			"valid,"
			"parent "
			") VALUES ("
			+escape(id)+","
			+escape(current_user())+","
			"now(),"
			"1,"
			+escape(a.parent)
			+")";
		run_cmd(db,q);
	}
	//Subsystem_editor page;
	//page.id={id};
	/*make_page(
		o,
		"New subsystem",
		redirect_to(page)
	);*/
	vector<pair<string,string>> v;
	v|=pair<string,string>("edit_date","now()");
	v|=pair<string,string>("edit_user",escape(current_user()));
	#define X(A,B) if(""#B==string("part_number")) v|=part_entry(db,a.parent,a.B); else v|=pair<string,string>(""#B,escape(a.B));
	SUBSYSTEM_NEW_DATA_ITEMS(X)
	#undef X
	v|=pair<string,string>{"subsystem_id",escape(id)};
	insert(db,"subsystem_info",v);
	make_page(
		o,
		"Subsystem edit",
		redirect_to([=]()->URL{
			//if(a.after) return *a.after;
			Subsystem_editor page;
			page.id=id;//Subsystem_id{a.subsystem_id};
			return URL{to_query(page)};
		}())
	);
}

string parts_of_subsystem(DB db,Request const& page,Subsystem_id id){
	auto q=qm<Part_id,Part_state,int>(
		db,
		"SELECT part_id,part_state,qty "
		"FROM part_info "
		"WHERE (id) IN "
			"(SELECT MAX(id) FROM part_info GROUP BY part_id) "
			"AND valid AND subsystem="+escape(id)
	);
	return h2("Parts directly in subsystem")+as_table(db,page,vector<Label>{"Part","State","Qty"},q);
}

string subsystems_of_subsystem(DB db,Request const& page,Subsystem_id subsystem){
	return h2("Subsystems directly in this subsystem")+as_table(
		db,
		page,
		vector<Label>{"Subsystem","Prefix"},
		qm<Subsystem_id,Subsystem_prefix>(
			db,
			"SELECT subsystem_id,prefix "
			"FROM subsystem_info "
			"WHERE "
				"valid AND "
				"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) AND "
				"parent="+escape(subsystem)
		)
	);
}

Subsystem_data subsystem_data(DB db,Subsystem_id id){
	vector<string> data_col_names;
	#define X(A,B) data_col_names|=""#B;
	SUBSYSTEM_DATA(X)
	#undef X

	auto q=query(
		db,
		"SELECT "+join(",",data_col_names)+
		" FROM subsystem_info "
		"WHERE subsystem_id="+escape(id)+
		" ORDER BY edit_date DESC LIMIT 1"
	);
	Subsystem_data current{};
	if(q.size()==0){
		current.valid=1;
	}else{
		assert(q.size()==1);
		auto row=q[0];
		unsigned i=0;
		#define X(A,B) { \
			if(row[i]) current.B=parse((A*)0,*row[i]); \
			i++;\
		}
		SUBSYSTEM_DATA(X)
		#undef X
	}

	return current;
}

void inner(ostream& o,Subsystem_editor const& a,DB db){
	string user = current_user();
	if (user == "no_user") {
		cout << "Location: /cgi-bin/login.cgi\n\n";
	}
	vector<string> info_col_names;
	#define X(A,B) info_col_names|=""#B;
	SUBSYSTEM_INFO_ROW(X)
	#undef X

	auto current=subsystem_data(db,a.id);
	make_page(
		o,
		as_string(current.name)+" Subsystem",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\"Subsystem_edit\">"
		"<input type=\"hidden\" name=\"subsystem_id\" value=\""+escape(a.id)+"\">"+
		after_done()
		+input_table([=](){
			vector<Input> r;
			#define X(A,B) r|=show_input(db,""#B,current.B);
			SUBSYSTEM_DATA(X)
			#undef X
			return r;
		}())+
		"<br><input type=\"submit\">"+
		"</form>"
		+link([=](){ BOM r; r.subsystem=a.id; return r; }(),"BOM")
		+" "+link([=](){ Subsystem_duplicate r; r.subsystem=a.id; return r; }(),"Duplicate")
		+h2("Overview")
		+[=](){
			stringstream ss;
			Subsystem_new sub_new;
			sub_new.parent=a.id;
			ss<<link(sub_new,"New Subsystem")<<" ";
			Part_new part_new;
			part_new.subsystem=a.id;
			ss<<link(part_new,"New Part");
			return ss.str();
		}()
		+"<table border><tr>"+th("Name")+th("Status")+"</tr>"+indent_sub_table(db,0,a.id,{})+"</table>"
		+parts_of_subsystem(db,a,a.id)
		+subsystems_of_subsystem(db,a,a.id)
		+completion_est(db,a.id)
		+h2("History")
		+make_table(
			a,
			{
				//"edit_date","edit_user","id","name","subsystem_id","valid","parent","prefix"
				#define X(A,B) ""#B,
				SUBSYSTEM_INFO_ROW(X)
				#undef X
			},
			query(
				db,
				"SELECT "+join(",",info_col_names)+
				" FROM subsystem_info WHERE subsystem_id="+escape(a.id)
			)
		)
	);
}

void inner(ostream& o,Subsystem_edit const& a,DB db){
	string user = current_user();
	if (user == "no_user") {
		cout << "Location: /cgi-bin/login.cgi\n\n";
	}
	vector<pair<string,string>> v;
	v|=pair<string,string>("edit_date","now()");
	v|=pair<string,string>("edit_user",escape(current_user()));
	#define X(A,B) if(""#B==string("part_number")) v|=part_entry(db,a.parent,a.B); else v|=pair<string,string>(""#B,escape(a.B));
	SUBSYSTEM_EDIT_DATA_ITEMS(X)
	#undef X
	insert(db,"subsystem_info",v);
	make_page(
		o,
		"Subsystem edit",
		redirect_to([=]()->URL{
			if(a.after) return *a.after;
			Subsystem_editor page;
			page.id=Subsystem_id{a.subsystem_id};
			return URL{to_query(page)};
		}())
	);
}

bool make_duplicate(DB db,Subsystem_id new_parent,Part_id current){
	//basically, just do the same thing as in the part duplicate page, except set the parent to something different
	auto data=part_data(db,current);
	if(!data){
		return 1;
	}
	auto id=Part_id{new_item(db,"part")};
	data->subsystem=new_parent;
	insert_part_data(db,id,*data);
	return 0;
}

void insert_subsystem_data(DB db,Subsystem_id id,Subsystem_data data){
	vector<pair<string,string>> items;
	items|=pair<string,string>("edit_user",escape(current_user()));
	items|=pair<string,string>("edit_date","now()");
	items|=pair<string,string>("subsystem_id",escape(id));
	#define X(A,B) items|=pair<string,string>(""#B,escape(data.B));
	SUBSYSTEM_DATA(X)
	#undef X
	insert(db,"subsystem_info",items);
}

Subsystem_id make_duplicate_local(DB db,optional<Subsystem_id> new_parent,Subsystem_id current){
	auto data=subsystem_data(db,current);
	auto id=Subsystem_id{new_item(db,"subsystem")};
	data.parent=new_parent;
	insert_subsystem_data(db,id,data);
	return id;
}

vector<Subsystem_id> subsystems_of(DB db,Subsystem_id a){
	return q1<Subsystem_id>(
		db,
		"SELECT subsystem_id "
		"FROM subsystem_info "
		"WHERE "
			"valid "
			"AND id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) "
			"AND parent="+escape(a)
	);
}

vector<Part_id> parts_of(DB db,Subsystem_id a){
	return q1<Part_id>(
		db,
		"SELECT part_id "
		"FROM part_info "
		"WHERE "
			"valid "
			"AND id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
			"AND subsystem="+escape(a)
	);
}

Subsystem_id make_duplicate(DB db,optional<Subsystem_id> new_parent,Subsystem_id current,set<Subsystem_id> visited){
	//returns the id of the copy.

	if(visited.count(current)){
		throw "Error: Loop detected.  Not duplicating.";
	}
	visited|=current;

	//cout<<"Current:"<<current<<"\n";
	//first, duplicate the local data for this subsystem
	auto new_subsystem=make_duplicate_local(db,new_parent,current);
	//cout<<"New:"<<new_subsystem<<"\n";

	//next, recurse into each of the child susbsystems
	for(auto x:subsystems_of(db,current)){
		//cout<<"Rec into:"<<x<<"\n";
		make_duplicate(db,new_subsystem,x,visited);
	}

	//finally, duplicate parts
	bool error=0;
	for(auto x:parts_of(db,current)){
		//cout<<"Copy part:"<<x<<"\n";
		error|=make_duplicate(db,new_subsystem,x);
	}

	return new_subsystem;
}

optional<Subsystem_id> get_parent(DB db,Subsystem_id id){
	auto q=qm<optional<Subsystem_id>>(
		db,
		"SELECT parent "
		"FROM subsystem_info "
		"WHERE "
			"valid "
			"AND id IN (SELECT MAX(id) FROM subsystem_info WHERE subsystem_id="+escape(id)+")"
	);
	if(q.size()==0) return std::nullopt;
	assert(q.size()==1);
	return get<0>(q[0]);
}

void inner(ostream& o,Subsystem_duplicate const& a,DB db){
	string user = current_user();
	if (user == "no_user") {
		cout << "Location: /cgi-bin/login.cgi\n\n";
	}
	//duplicate depth-first
	//first, check that the given subsystem exists
	//next, duplicate 
	auto parent=get_parent(db,a.subsystem);
	auto new_id=make_duplicate(db,parent,a.subsystem,{});

	make_page(
		o,
		"Subsystem duplicate",
		redirect_to([=](){
			Subsystem_editor r;
			r.id=new_id;
			return r;
		}())
	);
}
