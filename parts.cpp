#include "util.h"
#include "data_types.h"
#include "auth.h"

using namespace std;

#include "queries.h"

string link(Request req,string body){
	stringstream ss;
	ss<<"<a href=\"?"<<to_query(req)<<"\">"<<body<<"</a>";
	return ss.str();
}

string title_end(){ return "1425 Parts System"; }

string nav(){
	return ""
		#define X(A) +[]()->string{ \
			if(sizeof(A)==1) return link(A{},""#A)+" "; \
			return ""; \
		}()
		PAGES(X)
		#undef X
	+"<br>"
	;
}

string make_page(string heading,string main_body){
	string name=heading+" - "+title_end();
	return html(
		head(
			title(name)
		)+
		body(
			h1(heading)+nav()+main_body
		)
	);

}

string inner(Home const& a,DB){
	return make_page("Home","");
}

string make_table(vector<string> labels,vector<vector<optional<string>>> const& a){
	stringstream ss;
	ss<<"<table border>";
	ss<<"<tr>";
	for(auto elem:labels) ss<<th(elem);
	ss<<"</tr>";
	for(auto row:a){
		ss<<"<tr>";
		for(auto elem:row){
			ss<<td(as_string(elem));
		}
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

string make_table(vector<vector<optional<string>>> const& a){
	stringstream ss;
	ss<<"<table border>";
	for(auto row:a){
		ss<<"<tr>";
		for(auto elem:row){
			ss<<td(as_string(elem));
		}
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

string show_table(DB db,Table_name name){
	auto columns=firsts(query(db,"DESCRIBE "+name));
	stringstream ss;
	ss<<h2(name);
	ss<<"<table border>";
	ss<<"<tr>";
	for(auto elem:columns) ss<<th(as_string(elem));
	ss<<"</tr>";
	for(auto row:query(db,"SELECT * FROM "+name)){
		ss<<"<tr>";
		for(auto elem:row){
			ss<<td(as_string(elem));
		}
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

vector<int> get_ids(DB db,Table_name table){
	vector<int> r;
	for(auto row:query(db,"SELECT id FROM "+table)){
		r|=stoi(*row.at(0));
	}
	return r;
}

string inner(Subsystems const& a,DB db){
	return make_page(
		"Subsystems",
		[=](){
			stringstream ss;
			for(auto id:get_ids(db,"subsystem")){
				ss<<link(Subsystem_editor{id},"Edit "+as_string(id));
			}
			return ss.str();
		}()+
		show_table(db,"subsystem")+
		show_table(db,"subsystem_info")
	);
}

template<typename T>
string redirect_to(T t){
	stringstream ss;
	//ss<<"<meta http-equiv = \"refresh\" content = \"2; url = ";
	ss<<"<meta http-equiv = \"refresh\" content = \"0; url = ";
	ss<<"?"<<to_query(t);
	ss<<"\" />";
	ss<<"You should have been automatically redirected to:"<<t<<"\n";
	return ss.str();
}

template<typename T>
string inner_new(DB db,Table_name table){
	run_cmd(db,"INSERT INTO "+table+" VALUES ()");
	auto q=query(db,"SELECT LAST_INSERT_ID()");
	//PRINT(q);
	assert(q.size()==1);
	assert(q[0].size()==1);
	assert(q[0][0]);
	auto id=stoi(*q[0][0]);

	return make_page(
		"Subsystem new",
		redirect_to(T{id})
	);
}

string inner(Subsystem_new const&,DB db){
	return inner_new<Subsystem_editor>(db,"subsystem");
}

string inner(Subsystem_editor const& a,DB db){
	auto q=query(
		db,
		"SELECT name,valid FROM subsystem_info "
		"WHERE subsystem_id="+as_string(a.id)+
		" ORDER BY edit_date DESC LIMIT 1"
	);
	string name;
	bool valid;
	if(q.size()==0){
		name="";
		valid=1;
	}else{
		assert(q.size()==1);
		assert(q[0].size()==2);
		name=*q[0][0];
		valid=stoi(*q[0][1]);
	}
	return make_page(
		"Subsystem editor",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\"Subsystem_edit\">"
		"<input type=\"hidden\" name=\"subsystem_id\" value=\""+to_string(a.id)+"\">"
		"<br>Name:<input type=\"text\" name=\"name\" value=\""+name+"\">"+
		"<br>Valid:<input type=\"checkbox\" name=\"valid\" "+
			[=](){ if(valid) return "checked=on"; return ""; }()+"\">"+
		"<br><input type=\"submit\">"+
		"</form>"
		+h2("History")
		+make_table(
			{"edit_date","edit_user","id","name","subsystem_id","valid"},
			query(db,"SELECT * FROM subsystem_info WHERE subsystem_id="+as_string(a.id))
		)
	);
}

string inner(Subsystem_edit const& a,DB db){
	vector<pair<string,string>> v;
	v|=pair<string,string>("edit_date","now()");
	v|=pair<string,string>("edit_user",escape("user1"));
	#define X(A,B) v|=pair<string,string>(""#B,escape(a.B));
	SUBSYSTEM_EDIT_ITEMS(X)
	#undef X
	auto q="INSERT INTO subsystem_info ("
		+join(",",firsts(v))
		+") VALUES ("
		+join(",",seconds(v))
		+")";
	//PRINT(q);
	run_cmd(db,q);
	return make_page(
		"Subsystem edit",
		redirect_to(Subsystem_editor{a.subsystem_id})
	);
}

string inner(Part_new const& a,DB db){
	return inner_new<Part_editor>(db,"part");
}

string inner(Parts const&,DB db){
	return make_page(
		"Parts",
		[=](){
			stringstream ss;
			for(auto id:get_ids(db,"part")){
				ss<<link(Part_editor{id},"Edit "+as_string(id))<<"<br>";
			}
			return ss.str();
		}()+
		show_table(db,"part")+
		show_table(db,"part_info")
	);
}

string inner(Part_editor const& a,DB db){
	vector<string> data_cols{
		#define X(A,B) ""#B,
		PART_DATA(X)
		#undef X
	};
	string area_lower="part";
	string area_cap="Part";
	auto q=query(
		db,
		"SELECT " +join(",",data_cols)+ " FROM "+area_lower+"_info "
		"WHERE "+area_lower+"_id="+as_string(a.id)+
		" ORDER BY edit_date DESC LIMIT 1"
	);
	Part_data current{};
	/*string name;
	bool valid;*/
	if(q.size()==0){
		//name="";
		current.valid=1;
	}else{
		assert(q.size()==1);
		assert(q[0].size()==data_cols.size());
		int i=0;
		#define X(A,B) current.B=parse((A*)nullptr,*q[0][i]); i++;
		PART_DATA(X)
		#undef X
	}
	vector<string> all_cols=vector<string>{"edit_date","edit_user","id",area_lower+"_id"}+data_cols;
	return make_page(
		area_cap+" editor",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\""+area_cap+"_edit\">"
		"<input type=\"hidden\" name=\""+area_lower+"_id\" value=\""+to_string(a.id)+"\">"
		#define X(A,B) "<br>"+show_input(db,""#B,current.B)+
		PART_DATA(X)
		#undef X
		"<br><input type=\"submit\">"+
		"</form>"
		+h2("History")
		+make_table(
			all_cols,
			query(db,"SELECT "+join(",",all_cols)+" FROM "+area_lower+"_info WHERE "+area_lower+"_id="+as_string(a.id))
		)
	);
}

string inner(Meeting_editor const& a,DB db){
	vector<string> data_cols{
		#define X(A,B) ""#B,
		MEETING_DATA(X)
		#undef X
	};
	string area_lower="meeting";
	string area_cap="Meeting";
	auto q=query(
		db,
		"SELECT " +join(",",data_cols)+ " FROM "+area_lower+"_info "
		"WHERE "+area_lower+"_id="+as_string(a.id)+
		" ORDER BY edit_date DESC LIMIT 1"
	);
	Meeting_data current{};
	/*string name;
	bool valid;*/
	if(q.size()==0){
		//name="";
		current.valid=1;
	}else{
		assert(q.size()==1);
		assert(q[0].size()==data_cols.size());
		int i=0;
		#define X(A,B) current.B=parse((A*)nullptr,*q[0][i]); i++;
		MEETING_DATA(X)
		#undef X
	}
	vector<string> all_cols=vector<string>{"edit_date","edit_user","id",area_lower+"_id"}+data_cols;
	return make_page(
		area_cap+" editor",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\""+area_cap+"_edit\">"
		"<input type=\"hidden\" name=\""+area_lower+"_id\" value=\""+to_string(a.id)+"\">"
		#define X(A,B) "<br>"+show_input(db,""#B,current.B)+
		MEETING_DATA(X)
		#undef X
		"<br><input type=\"submit\">"+
		"</form>"
		+h2("History")
		+make_table(
			all_cols,
			query(db,"SELECT "+join(",",all_cols)+" FROM "+area_lower+"_info WHERE "+area_lower+"_id="+as_string(a.id))
		)
	);
}

string inner(Meeting_new const&,DB db){
	return inner_new<Meeting_editor>(db,"meeting");
}

string inner(Calendar const&,DB db){
	return make_page(
		"Calendar",
		show_table(db,"meeting")
		+show_table(db,"meeting_info")
	);
}

string inner(Part_edit const& a,DB db){
	string area_lower="part";
	string area_cap="Part";

	vector<pair<string,string>> v;
	v|=pair<string,string>("edit_date","now()");
	v|=pair<string,string>("edit_user",escape("user1"));
	#define X(A,B) v|=pair<string,string>(""#B,escape(a.B));
	PART_EDIT_ITEMS(X)
	#undef X
	auto q="INSERT INTO "+area_lower+"_info ("
		+join(",",firsts(v))
		+") VALUES ("
		+join(",",seconds(v))
		+")";
	run_cmd(db,q);
	return make_page(
		area_cap+" edit",
		redirect_to(Part_editor{a.part_id})
	);
}

string inner(Meeting_edit const& a,DB db){
	string area_lower="meeting";
	string area_cap="Meeting";

	vector<pair<string,string>> v;
	v|=pair<string,string>("edit_date","now()");
	v|=pair<string,string>("edit_user",escape("user1"));
	#define X(A,B) v|=pair<string,string>(""#B,escape(a.B));
	MEETING_EDIT_ITEMS(X)
	#undef X
	auto q="INSERT INTO "+area_lower+"_info ("
		+join(",",firsts(v))
		+") VALUES ("
		+join(",",seconds(v))
		+")";
	run_cmd(db,q);
	return make_page(
		area_cap+" edit",
		redirect_to(Meeting_editor{a.meeting_id})
	);
}

string inner(Error const& a,DB db){
	return make_page(
		"Error",
		a.s
	);
}

#define EMPTY_PAGE(X) string inner(X x,DB db){ \
	return make_page(\
		""#X,\
		as_string(x)+p("Under construction")\
	); \
}
//EMPTY_PAGE(Meeting_editor)
//EMPTY_PAGE(Meeting_edit)
#undef EMPTY_PAGE


string run(Request req,DB db){
	#define X(A) if(holds_alternative<A>(req)) return inner(get<A>(req),db);
	PAGES(X)
	X(Error)
	#undef X
	PRINT(req)
	if(req==Home{}){
		nyi;
	}
	nyi
	//return run(
}

template<
	#define X(A) typename A,
	PAGES(X)
	#undef X
	typename Z
>
std::variant<
	#define X(A) A,
	PAGES(X)
	#undef X
	Z
> rand(const std::variant<
	#define X(A) A,
	PAGES(X)
	#undef X
	Z
>*){
	int i=1;
	#define X(A) i++;
	PAGES(X)
	#undef X
	int chosen=rand()%i;
	int j=0;
	#define X(A) if(chosen==j) return rand((A*)0); j++;
	PAGES(X)
	#undef X
	return rand((Z*)0);
}

optional<Request> parse_referer(const char *s){
	if(!s) return {};
	auto at=s;
	while(*at && *at!='?') at++;
	if(!*at) return {};
	at++;
	//cout<<"At: \""<<at<<"\"\n";
	return parse_query(at);
}

struct DB_connection{
	DB db;

	~DB_connection(){
		mysql_close(db);
	}
};

int main1(int argc,char **argv,char **envp){
	cout<<"Content-type: text/html\n\n";

	/*for(int i=0;envp[i];i++){
		cout<<"env:"<<envp[i]<<"<br>\n";
	}*/

	/*auto g1=getenv("HTTP_REFERER");
	auto p=parse_referer(g1);
	cout<<"ref from:"<<p<<"\n";*/

	DB db=mysql_init(NULL);
	assert(db);

	auto a=auth();
	auto r1=mysql_real_connect(
		db,
		a.host.c_str(),
		a.user.c_str(),
		a.pass.c_str(),
		a.db.c_str(),
		0,NULL,0
	);

	if(!r1){
		cout<<"Connect fail:"<<mysql_error(db)<<"\n";
		exit(1);
	}
	DB_connection con{db};

	check_database(db);

	auto g=getenv("QUERY_STRING");
	if(g){
		cout<<run(parse_query(g),db);
		return 0;
	}
	for(auto _:range(100)){
		(void)_;
		auto r=rand((Request*)nullptr);
		//PRINT(r);
		//PRINT(to_query(r));
		auto p=parse_query(to_query(r));
		if(p!=r){
			PRINT(p);
			PRINT(r);
			diff(p,r);
		}
		assert(p==r);
		cout<<run(r,db);
	}
	auto q=parse_query("");
	PRINT(q);
	auto s=run(q,db);
	PRINT(s);
	return 0;
}

int main(int argc,char **argv,char **envp){
	try{
		return main1(argc,argv,envp);
	}catch(const char *s){
		cout<<"Caught:"<<s<<"\n";
	}
}
