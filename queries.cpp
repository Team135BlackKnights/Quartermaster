#include "queries.h"
#include<string.h>

using namespace std;

//move to util
template<typename A,typename B>
std::vector<std::pair<std::optional<A>,std::optional<B>>> zip_extend(std::vector<A> const& a,std::vector<B> const& b){
	std::vector<std::pair<std::optional<A>,std::optional<B>>> r;
	for(auto i:range(max(a.size(),b.size()))){
		r|=make_pair(
			[=]()->optional<A>{ if(i<a.size()) return a[i]; return {}; }(),
			[=]()->optional<B>{ if(i<b.size()) return b[i]; return {}; }()
		);
	}
	return r;
}

//move to util.
template<typename T>
void diff(vector<T> const& a,vector<T> const& b){
	if(a.size()!=b.size()){
		auto z=zip_extend(a,b);
		for(auto [a1,b1]:zip(a,b)){
			if(a1!=b1){
				cout<<"a:"<<a1<<"\n";
				cout<<"b:"<<b1<<"\n";
			}
		}
		return;
	}

	for(auto [a1,b1]:zip(a,b)){
		if(a1!=b1){
			cout<<"a:"<<a1<<"\n";
			cout<<"b:"<<b1<<"\n";
		}
	}
}

Page::~Page(){}

Table_type read(DB db,string const& name){
	auto q=query(db,"DESCRIBE "+name);
	Table_type r;
	for(auto elem:q){
		//PRINT(elem);
		assert(elem.size()==6);
		auto name=*elem[0];
		auto type=*elem[1];
		auto primary=*elem[3];
		//PRINT(primary);
		r|=make_pair(name,make_pair(type,primary=="PRI"));
	}
	return r;
}

using Table_name=string;

void check_table(DB db,Table_name const& name,Table_type const& type){
	auto r=read(db,name);
	if(r!=type){
		cout<<"Table mismatch:"<<name<<"\n";
		PRINT(r);
		PRINT(type);
		diff(r,type);
	}
	assert(r==type);
}

set<Table_name> show_tables(DB db){
	auto q=query(db,"SHOW TABLES");
	return to_set(mapf(
		[](auto a)->Table_name{
			assert(a.size()==1);
			assert(a[0]);
			return *a[0];
		},
		q
	));
}

string to_db_type(const bool*){ return "tinyint(1)"; }
string to_db_type(const unsigned*){ return "int(11) unsigned"; }
string to_db_type(const string*){ return "varchar(100)"; }
string to_db_type(const double*){ return "double"; }

void create_table(DB db,Table_name const& name,Table_type const& type){
	run_cmd(
		db,
		[=](){
			stringstream ss;
			ss<<"CREATE TABLE "<<name<<"(";
			for(auto [last,p]:mark_last(type)){
				auto [k,v]=p;
				ss<<k<<" "<<v.first;
				if(v.second){
					ss<<" unique NOT NULL primary key AUTO_INCREMENT";
				}
				if(!last) ss<<",";
			}
			ss<<")";
			return ss.str();
		}()
	);
}

void check_database(DB db){
	map<Table_name,Table_type> tables;
	#define COLUMN_INFO(A,B) {""#B,{to_db_type((A*)nullptr),""#B==string{"id"}}},
	#define TABLE(NAME,ITEMS) tables[NAME]={ ITEMS(COLUMN_INFO)};
	TABLE("meeting",MEETING_ROW)
	TABLE("meeting_info",MEETING_INFO_ROW)
	TABLE("part",PART_ROW)
	TABLE("part_info",PART_INFO_ROW)
	TABLE("subsystem",SUBSYSTEM_ROW)
	TABLE("subsystem_info",SUBSYSTEM_INFO_ROW)

	auto t=show_tables(db);
	//PRINT(t);

	for(auto table:tables){
		if(!t.count(table.first)){
			cout<<"Missing table:"<<table.first<<"\n";
			create_table(db,table.first,table.second);
		}else{
			check_table(db,table.first,table.second);
		}
	}
}

using P=map<string,vector<string>>;

string to_query(map<string,string> const& m){
	stringstream ss;
	for(auto at=begin(m);at!=end(m);++at){
		auto [k,v]=*at;
		//could try to do some fancier encoding in here.
		ss<<k<<"="<<v;
		auto next=at;
		next++;
		if(next!=end(m)){
			ss<<"&";
		}
	}
	return ss.str();
}

unsigned parse(const unsigned*,string const& s){ return stoi(s); }

bool parse(const bool*,string const& s){
	//This is how the database keeps track of it
	if(s=="1") return 1;
	if(s=="0") return 0;

	//This is how HTML forms make it appear
	if(s=="on") return 1;
	if(s=="off") return 0;

	PRINT(s);
	nyi
}

string parse(const string*,string const& s){ return s; }

float parse(const float*,string const& s){
	return stof(s);
}

double parse(const double*,string const& s){
	return stod(s);
}

unsigned rand(const unsigned*){ return rand()%100; }

#define STR(X) ""#X

#define PARSE_ITEM(A,B) { \
	auto f=p.find(""#B); \
	if(f==p.end()){\
		if(""#A==string("bool")){\
			r.B=A{};\
		}else if(strstr(""#A,"optional")){\
			r.B=A{};\
		}else{\
			throw string()+"Failed to find:"#B+as_string(p);\
		}\
	}else{\
		if(f->second.size()!=1){ throw "Multiple values for "#B; }\
		try{\
			r.B=parse((A*)nullptr,f->second[0]);\
		}catch(...){\
			throw string()+"Failed to parse \""#B+"\" as an "#A+": \""+f->second[0]+"\""; \
		}\
		p=without_key(p,string(""#B));\
	}\
}

#define TO_Q(A,B) r[""#B]=as_string(a.B);
#define RAND(A,B) r.B=rand((const A*)nullptr);
#define INST_EQ(A,B) if(a.B!=b.B) return 0;
#define SHOW(A,B) { o<<""#B<<":"<<a.B<<" "; }
#define DIFF(A,B) if(a.B!=b.B) cout<<""#B<<":"<<a.B<<" "<<b.B<<"\n";

#define DEF_OPTION(T,ITEMS)\
	bool operator==(T const& a,T const& b){\
		ITEMS(INST_EQ)\
		return 1;\
	}\
	bool operator!=(T const& a,T const& b){\
		return !(a==b);\
	}\
	std::ostream& operator<<(std::ostream& o,T const& a){\
		o<<STR(T)<<"( ";\
		ITEMS(SHOW)\
		return o<<")";\
	}\
	T rand(const T*){\
		T r;\
		RAND(std::optional<std::string>,sort_by)\
		RAND(std::optional<std::string>,sort_order)\
		ITEMS(RAND)\
		return r;\
	}\
	string to_query(T const& a){\
		map<string,string> r;\
		r["p"]=STR(T);\
		ITEMS(TO_Q)\
		r["sort_by"]=as_string(a.sort_by);\
		r["sort_order"]=as_string(a.sort_order);\
		return to_query(r);\
	}\
	optional<T> parse_query(const T*,P const& p1){\
		auto p=p1;\
		if(p["p"]!=vector<string>{STR(T)}){ \
			return {};\
		}\
		p=without_key(p,string{"p"});\
		T r;\
		PARSE_ITEM(optional<string>,sort_by)\
		PARSE_ITEM(optional<string>,sort_order)\
		ITEMS(PARSE_ITEM)\
		if(p.size()) return {};\
		return r;\
	}\
	void diff(T const& a,T const& b){\
		ITEMS(DIFF)\
	}

DEF_OPTION(Home,HOME_ITEMS)
DEF_OPTION(Subsystems,SUBSYSTEMS_ITEMS)
DEF_OPTION(Subsystem_new,SUBSYSTEM_NEW_ITEMS)
DEF_OPTION(Subsystem_editor,SUBSYSTEM_EDITOR_ITEMS)
DEF_OPTION(Subsystem_edit,SUBSYSTEM_EDIT_ITEMS)
DEF_OPTION(Parts,PARTS_ITEMS)
DEF_OPTION(Part_new,PART_NEW_ITEMS)
DEF_OPTION(Part_editor,PART_EDITOR_ITEMS)
DEF_OPTION(Part_edit,PART_EDIT_ITEMS)
DEF_OPTION(Calendar,CALENDAR_ITEMS)
DEF_OPTION(Meeting_new,MEETING_NEW_ITEMS)
DEF_OPTION(Meeting_editor,MEETING_EDITOR_ITEMS)
DEF_OPTION(Meeting_edit,MEETING_EDIT_ITEMS)
DEF_OPTION(Error,ERROR_ITEMS)
DEF_OPTION(By_user,BY_USER_ITEMS)
DEF_OPTION(Machines,MACHINES_ITEMS)
DEF_OPTION(Orders,ORDERS_ITEMS)
DEF_OPTION(Machine_page,MACHINE_ITEMS)
DEF_OPTION(State,STATE_ITEMS)

int hex_digit(char c){
	if(c>='0' && c<='9') return c-'0';
	if(c>='a' && c<='f') return c-'a'+10;
	if(c>='A' && c<='F') return c-'A'+10;
	nyi
}

char from_hex(char a,char b){
	return (hex_digit(a)<<4)+hex_digit(b);
}

string decode(string const& s){
	//Remove %20, etc. style escapes
	//There's got to be a library function for this somewhere
	stringstream ss;
	for(auto at=begin(s);at!=end(s);){
		if(*at=='%'){
			at++;
			assert(at!=end(s));
			auto b1=*at;
			at++;
			assert(at!=end(s));
			auto b2=*at;
			ss<<from_hex(b1,b2);
			at++;
		}else if(*at=='+'){
			ss<<" ";
			at++;
		}else{
			ss<<*at;
			at++;
		}
	}
	return ss.str();
}

map<string,vector<string>> parse_query_string(string const& s){
	map<string,vector<string>> r;
	auto at=begin(s);
	auto end=std::end(s);
	while(at!=end){
		auto start=at;
		while(at!=end && *at!='='){
			at++;
		}
		string key(start,at);
		if(at==end) throw "Error: Invalid cgi query string.  Variable with no value specified.  Name:"+key+" Whole:"+s; //assert(at!=end);
		at++;
		start=at;
		while(at!=end && *at!='&'){
			at++;
		}
		string value(start,at);
		r[key]|=decode(value);
		if(at!=end) at++; //skip the '&'
	}
	return r;
}

Request parse_query(string const& s){
	auto m=parse_query_string(s);
	if(m.empty()) return Home{};
	//PRINT(m);
	#define X(A) try{ \
		auto p=parse_query((A*)nullptr,m); if(p) return *p; \
	}catch(string s){\
		Error page;\
		page.s="Invalid argument"+s;\
		return page;\
	}
	PAGES(X)
	X(Error)
	#undef X
	Error r;
	r.s="Unparsable request:"+as_string(m);
	return r;
}

string link(Request const& req,string const& body){
	stringstream ss;
	ss<<"<a href=\"?"<<to_query(req)<<"\">"<<body<<"</a>";
	return ss.str();
}

