#ifndef QUERIES_H
#define QUERIES_H

#include "data_types.h"

#define SUBSYSTEM_DATA(X)\
	X(bool,valid)\
	X(std::string,name)\

#define SUBSYSTEM_ROW(X)\
	X(Id,id)\

#define SUBSYSTEM_INFO_ROW(X)\
	X(Id,id)\
	X(Id,subsystem_id)\
	X(User,edit_user)\
	X(Datetime,edit_date)\
	SUBSYSTEM_DATA(X)

#define PART_ROW(X)\
	X(Id,id)\

#define PART_DATA(X)\
	X(bool,valid)\
	X(Subsystem_id,subsystem)\
	X(std::string,name)\
	X(std::string,part_number)\
	X(std::string,incremental_number)\
	X(Part_state,part_state)\
	X(std::string,length)\
	X(std::string,width)\
	X(std::string,thickness)\
	X(Material,material)\
	X(unsigned,qty)\
	X(Decimal,time)\
	X(Date,manufacture_date)\
	X(std::string,who_manufacure)\
	X(Machine,machine)\
	X(std::string,place)\
	X(std::string,bent)\
	X(Bend_type,bend_type)\
	X(URL,drawing_link)\
	X(URL,cam_link)\
	X(Supplier,part_supplier)\
	X(URL,part_link)\
	X(Date,arrival_date)\
	X(Decimal,price)

struct Part_data{
	PART_DATA(INST)
};

#define PART_INFO_ROW(X)\
	X(Id,id)\
	X(Id,part_id)\
	X(User,edit_user)\
	X(Datetime,edit_date)\
	PART_DATA(X)

#define MEETING_ROW(X)\
	X(Id,id)

#define MEETING_DATA(X)\
	X(bool,valid)\
	X(Date,date)\
	X(int,length)\
	X(std::string,color)\

struct Meeting_data{
	MEETING_DATA(INST)
};

#define MEETING_INFO_ROW(X)\
	X(Id,id)\
	X(Id,meeting_id)\
	X(User,edit_user)\
	X(Datetime,edit_date)\
	MEETING_DATA(X)

using Column_type=std::pair<std::string,bool>; //type and primary key
using Table_type=std::map<std::string,Column_type>;

Table_type read(DB db,std::string name);

using Table_name=std::string;

void check_table(DB,Table_name,Table_type);
std::set<Table_name> show_tables(DB);

std::string to_db_type(const bool*);
std::string to_db_type(const unsigned*);
std::string to_db_type(const std::string*);
std::string to_db_type(const double*);

void create_table(DB,Table_name,Table_type);

void check_database(DB);

using P=std::map<std::string,std::vector<std::string>>;

std::string to_query(std::map<std::string,std::string> m);

unsigned parse(const unsigned*,std::string);

bool parse(const bool*,std::string);
std::string parse(const std::string*,std::string);
float parse(const float*,std::string);
double parse(const double*,std::string);

unsigned rand(const unsigned*);

#define STR(X) ""#X

#define PARSE_ITEM(A,B) { \
	auto f=p.find(""#B); \
	if(f==p.end()){\
		if(""#A==string("bool")){\
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
#define RAND(A,B) rand((const A*)nullptr),
#define INST_EQ(A,B) if(a.B!=b.B) return 0;
#define SHOW(A,B) { o<<""#B<<":"<<a.B<<" "; }
#define DIFF(A,B) if(a.B!=b.B) cout<<""#B<<":"<<a.B<<" "<<b.B<<"\n";

#define DECL_OPTION(T,ITEMS)\
	struct T{ \
		ITEMS(INST) \
	};\
	bool operator==(T const& a,T const& b);\
	bool operator!=(T const& a,T const& b);\
	std::ostream& operator<<(std::ostream& o,T const& a);\
	T rand(const T*);\
	std::string to_query(T a);\
	std::optional<T> parse_query(const T*,P p);\
	void diff(T const& a,T const& b);\

//empty
#define HOME_ITEMS(X) 

#define T Home
DECL_OPTION(Home,HOME_ITEMS)
#undef T

#define SUBSYSTEMS_ITEMS(X)
#define T Subsystems
DECL_OPTION(Subsystems,SUBSYSTEMS_ITEMS)
#undef T

#define SUBSYSTEM_NEW_ITEMS(X)
DECL_OPTION(Subsystem_new,SUBSYSTEM_NEW_ITEMS)

#define SUBSYSTEM_EDITOR_ITEMS(X)\
	X(Id,id)
DECL_OPTION(Subsystem_editor,SUBSYSTEM_EDITOR_ITEMS)

#define SUBSYSTEM_EDIT_ITEMS(X)\
	X(Id,subsystem_id)\
	X(bool,valid)\
	X(std::string,name)

DECL_OPTION(Subsystem_edit,SUBSYSTEM_EDIT_ITEMS)

#define PARTS_ITEMS(X)
DECL_OPTION(Parts,PARTS_ITEMS)

#define PART_NEW_ITEMS(X)
DECL_OPTION(Part_new,PART_NEW_ITEMS)

#define PART_EDITOR_ITEMS(X)\
	X(Id,id)
DECL_OPTION(Part_editor,PART_EDITOR_ITEMS)
	
#define PART_EDIT_ITEMS(X)\
	X(Id,part_id)\
	PART_DATA(X)
DECL_OPTION(Part_edit,PART_EDIT_ITEMS)

#define CALENDAR_ITEMS(X)
DECL_OPTION(Calendar,CALENDAR_ITEMS)

#define MEETING_NEW_ITEMS(X)
DECL_OPTION(Meeting_new,MEETING_NEW_ITEMS)

#define MEETING_EDITOR_ITEMS(X)\
	X(Id,id)
DECL_OPTION(Meeting_editor,MEETING_EDITOR_ITEMS)

#define MEETING_EDIT_ITEMS(X)\
	X(Id,meeting_id)\
	MEETING_DATA(X)
DECL_OPTION(Meeting_edit,MEETING_EDIT_ITEMS)

#define ERROR_ITEMS(X)\
	X(std::string,s)
DECL_OPTION(Error,ERROR_ITEMS)

#define PAGES(X)\
	X(Home)\
	X(Subsystems)\
	X(Subsystem_new)\
	X(Subsystem_editor)\
	X(Subsystem_edit)\
	X(Parts)\
	X(Part_new)\
	X(Part_editor)\
	X(Part_edit)\
	X(Calendar)\
	X(Meeting_new)\
	X(Meeting_editor)\
	X(Meeting_edit)\

using Request=std::variant<
	#define X(A) A,
	PAGES(X)
	#undef X
	Error
>;
/*
int hex_digit(char c){
	if(c>='0' && c<='9') return c-'0';
	if(c>='a' && c<='f') return c-'a'+10;
	if(c>='A' && c<='F') return c-'A'+10;
	nyi
}

char from_hex(char a,char b){
	return (hex_digit(a)<<4)+hex_digit(b);
}

string decode(string s){
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

map<string,vector<string>> parse_query_string(string s){
	map<string,vector<string>> r;
	auto at=begin(s);
	auto end=std::end(s);
	while(at!=end){
		auto start=at;
		while(at!=end && *at!='='){
			at++;
		}
		string key(start,at);
		assert(at!=end);
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
*/
Request parse_query(std::string);

template<
	#define X(A) typename A,
	PAGES(X)
	#undef X
	typename Z
>
std::string to_query(std::variant<
	#define X(A) A,
	PAGES(X)
	#undef X
	Z
> a){
	#define X(A) if(std::holds_alternative<A>(a)) return to_query(std::get<A>(a));
	PAGES(X)
	X(Z)
	#undef X
	nyi
}

#endif
