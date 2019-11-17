#include "util.h"
#include "data_types.h"
#include "auth.h"

using namespace std;

#include "queries.h"

//TODO: Move to util
template<typename Func,typename T>
vector<T> filter(Func f,vector<T> v){
	vector<T> r;
	for(auto elem:v){
		if(f(elem)) r|=elem;
	}
	return r;
}

template<typename T>
vector<T> convert(vector<vector<optional<string>>> in){
	return mapf(
		[](auto x){
			assert(x.size()==1);
			return parse((T*)0,*x[0]);
		},
		in
	);
}

template<typename A,typename B>
vector<pair<A,B>> convert(vector<vector<optional<string>>> in){
	return mapf(
		[](auto x){
			assert(x.size());
	       		return make_pair(
				parse((A*)nullptr,*x[0]),
				parse((B*)nullptr,*x[1])
			);
		},
		in
	);
}

template<typename A,typename B,typename C>
vector<tuple<A,B,C>> convert(vector<vector<optional<string>>> a){
	return mapf(
		[](auto row){
			assert(row.size()==3);
			return make_tuple(
				parse((A*)0,*row[0]),
				parse((B*)0,*row[1]),
				parse((C*)0,*row[2])
			);
		},
		a
	);
}

template<typename T>
vector<T> q1(DB db,string query_string){
	auto q=query(db,query_string);
	//PRINT(q);
	return convert<T>(q);
}

template<typename A,typename B>
vector<pair<A,B>> q2(DB db,string query_string){
	auto q=query(db,query_string);
	return convert<A,B>(q);
}

template<typename A,typename B,typename C>
vector<tuple<A,B,C>> q3(DB db,string query_string){
	auto q=query(db,query_string);
	return convert<A,B,C>(q);
}

template<typename T>
vector<optional<T>> operator|=(vector<optional<T>>,T)nyi

template<typename T>
string pretty_td(DB,T t){
	return td(as_string(t));
}

string subsystem_name(DB db,Subsystem_id id){
	auto q=q1<string>(
		db,
		"SELECT name "
		"FROM subsystem_info "
		"WHERE (id) IN "
			"(SELECT MAX(id) FROM subsystem_info WHERE subsystem_id="+as_string(id)+") "
			"AND valid"
	);
	if(q.size()==0) return "No subsystem name found";
	if(q.size()>1){
		PRINT(id);
		PRINT(q);
		nyi
	}
	assert(q.size()==1);
	return q[0];
}

string pretty_td(DB db,Subsystem_id a){
	return td(link(Subsystem_editor{a},subsystem_name(db,a)));
}

string part_name(DB db,Part_id id){
	auto q=q1<string>(
		db,
		"SELECT name "
		"FROM part_info "
		"WHERE (name,edit_date) IN "
			"(SELECT name,MAX(edit_date) FROM part_info WHERE part_id="+as_string(id)+") "
			"AND valid"
	);
	//if(q.empty()) return "what?";
	//PRINT(q);
	assert(q.size()==1);
	return q[0];
}

string pretty_td(DB db, Part_id a){
	return td(link(Part_editor{a},part_name(db,a)));
}

template<typename A,typename B,typename C>
string as_table(DB db,vector<string> labels,vector<tuple<A,B,C>> const& a){
	stringstream ss;
	ss<<"<table border>";
	ss<<join("",mapf(th,labels));
	for(auto row:a){
		ss<<"<tr>";
		//ss<<td("aux");
		ss<<pretty_td(db,get<0>(row));
		ss<<pretty_td(db,get<1>(row));
		ss<<pretty_td(db,get<2>(row));
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

template<typename A,typename B>
string as_table(vector<string> labels,vector<pair<A,B>> const&){
	nyi
}

string as_table(vector<string> labels,vector<vector<std::string>> in){
	stringstream ss;
	ss<<"<table border>";
	ss<<"<tr>";
	ss<<join("",mapf(th,labels));
	ss<<"</tr>";
	for(auto row:in){
		ss<<"<tr>";
		for(auto item:row){
			ss<<td(as_string(item));
		}
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

string as_table(vector<string> labels,vector<vector<std::optional<string>>> in){
	stringstream ss;
	ss<<"<table border>";
	ss<<"<tr>";
	ss<<join("",mapf(th,labels));
	ss<<"</tr>";
	for(auto row:in){
		ss<<"<tr>";
		for(auto item:row){
			ss<<td(as_string(item));
		}
		ss<<"</tr>";
	}
	ss<<"</table>";
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

string show_current_subsystems(DB db){
	auto q=q2<Subsystem_id,string>(
		db,
                "SELECT subsystem_id,name FROM subsystem_info WHERE (subsystem_id,edit_date) IN (SELECT subsystem_id,MAX(edit_date) FROM subsystem_info GROUP BY subsystem_id) AND valid"
	);
	return as_table(
		{"subsystem_id","name"},
		mapf(
			[](auto x){
				vector<string> r;
				r|=as_string(x.first);
				r|=link(Subsystem_editor{x.first},x.second);
				return r;
			},
			q
		)
	);
}

string inner(Subsystems const& a,DB db){
	return make_page(
		"Subsystems",
		show_current_subsystems(db)/*+
		h2("Debug info")+
		[=](){
			stringstream ss;
			for(auto id:get_ids(db,"subsystem")){
				ss<<link(Subsystem_editor{id},"Edit "+as_string(id));
			}
			return ss.str();
		}()+
		show_table(db,"subsystem")+
		show_table(db,"subsystem_info")*/
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

string parts_of_subsystem(DB db,Subsystem_id id){
	auto q=q3<Part_id,Subsystem_id,string>(
		db,
		"SELECT part_id,subsystem,name "
		"FROM part_info "
		"WHERE (part_id,subsystem,edit_date) IN "
			"(SELECT part_id,subsystem,MAX(edit_date) FROM part_info GROUP BY part_id) "
			"AND valid AND subsystem="+as_string(id)
	);
	return as_table(db,{"part_id","subsystem","name"},q);
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
		"<input type=\"hidden\" name=\"subsystem_id\" value=\""+as_string(a.id)+"\">"
		"<br>Name:<input type=\"text\" name=\"name\" value=\""+name+"\">"+
		"<br>Valid:<input type=\"checkbox\" name=\"valid\" "+
			[=](){ if(valid) return "checked=on"; return ""; }()+"\">"+
		"<br><input type=\"submit\">"+
		"</form>"
		+h2("History")
		+make_table(
			{"edit_date","edit_user","id","name","subsystem_id","valid"},
			query(db,"SELECT * FROM subsystem_info WHERE subsystem_id="+as_string(a.id))
		)+parts_of_subsystem(db,a.id)
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

vector<pair<Id,string>> current_subsystems(DB db){
	auto q=query(db,"SELECT subsystem_id,name FROM subsystem_info WHERE (subsystem_id,edit_date) IN (SELECT subsystem_id,MAX(edit_date) FROM subsystem_info GROUP BY subsystem_id) AND valid");
	vector<pair<Id,string>> r;
	for(auto row:q){
		r|=make_pair(stoi(*row[0]),*row[1]);
	}
	return r;
}

string subsystem_name(DB db,Id subsystem_id){
	//query(db,"SELECT name FROM subsystem_info");
	//query(db,"SELECT subsystem_part_id,name FROM part_info WHERE (part_id,edit_date) IN (SELECT part_id,MAX(edit_date) FROM part_info GROUP BY part_id) AND valid"
	//auto q=query(db,"SELECT * FROM ");
	auto f=filter(
		[=](auto x){ return x.first==subsystem_id; },
		current_subsystems(db)
	);
	if(f.empty()){
		return "Not found ("+as_string(subsystem_id)+")";
	}
	assert(f.size()==1);
	return f[0].second;
}

string show_current_parts(DB db){
	auto q1=query(
		db,
		"SELECT * "
		"FROM part_info "
		"WHERE (part_id,subsystem,edit_date) IN (SELECT part_id,subsystem,MAX(edit_date) FROM part_info GROUP BY part_id) AND valid"
	);
	/*auto q1=q3<Part_id,Subsystem_id,string>(
		db,
		"SELECT part_id,subsystem,name FROM part_info WHERE (part_id,subsystem,edit_date) IN (SELECT part_id,subsystem,MAX(edit_date) FROM part_info GROUP BY part_id) AND valid"
	);*/
	return as_table({"part_id","subsystem_id","name"},q1);
	//auto q1=convert<Id,Id,string>(q);
	/*return as_table(
		{"part_id","subsystem_id","name"},
		mapf(
			[&](auto x){
				vector<string> r;
				r|=as_string(get<0>(x));
				r|=link(Subsystem_editor{get<1>(x)},subsystem_name(db,get<1>(x)));
				r|=link(Part_editor{get<0>(x)},as_string(get<2>(x)));
				return r;
			},
			q1
		)
	);*/
}


string inner(Parts const&,DB db){
	return make_page(
		"Parts",
		show_current_parts(db)+
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
		"<input type=\"hidden\" name=\""+area_lower+"_id\" value=\""+as_string(a.id)+"\">"
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
	//PRINT(q);
	auto s=run(q,db);
	//PRINT(s);
	return 0;
}

int main(int argc,char **argv,char **envp){
	try{
		return main1(argc,argv,envp);
	}catch(const char *s){
		cout<<"Caught:"<<s<<"\n";
	}
}
