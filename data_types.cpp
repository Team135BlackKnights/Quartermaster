#include "data_types.h"
#include "queries.h"
#include<cassert>

using namespace std;

string to_db_type(const int*){
	return "int(11)";
}

std::string to_db_type(const Part_number*){
	return "varchar(100)";
}

int rand(const int*){ return rand()%100; }

int parse(const int*,string const& s){ return stoi(s); }

string with_suggestions(string const& name,string const& value,vector<string> const& suggestions){
	stringstream ss;
	ss<<"<br>"<<name<<":<input name=\""<<name<<"\" list=\""<<name<<"\" value=\""<<value<<"\">";
	ss<<"<datalist id=\""<<name<<"\">";
	for(auto a:suggestions){
		ss<<"<option value=\""<<a<<"\">";
	}
	ss<<"</datalist>";
	return ss.str();
}

string show_input(DB,string const& name,string const& value){
	return "<br>"+name+":<input type=\"text\" name=\""+name+"\" value=\""+value+"\">";
}

string show_input(DB db,string const& name,int value){
	return show_input(db,name,as_string(value))+" (integer)";
}

string show_input(DB db,string const& name,double value){
	return show_input(db,name,as_string(value))+" (fraction)";
}

string show_input(DB db,string const& name,unsigned value){
	return show_input(db,name,as_string(value))+" (non-negative integer)";
}

string show_input(DB db,string const& name,bool value){
	return "<br>"+name+":<input type=\"checkbox\" name=\""+name+"\" "+
		[=](){ if(value) return "checked=on"; return ""; }()+"\">";
}

string escape(string const& s){
	stringstream ss;
	ss<<"\"";
	for(auto c:s){
		if(c=='"'){
			nyi
		}else{
			ss<<c;
		}
	}
	ss<<"\"";
	return ss.str();
}

string escape(int i){ return as_string(i); }

template<typename Value,typename Display>
string drop_down(string const& name,Value const& current,vector<pair<Value,Display>> const& v){
	stringstream ss;
	ss<<"<br>"<<name<<":";
	ss<<"<select name=\""<<name<<"\">";
	for(auto elem:v){
		ss<<"<option value=\""<<elem.first<<"\"";
		if(elem.first==current) ss<<"selected=selected";
		ss<<">"<<elem.second<<"</option><br>";
	}
	ss<<"</select>";
	return ss.str();
}

template<typename T>
string drop_down(string const& name,T const& current,vector<T> const& options){
	return drop_down(name,current,mapf([](auto x){ return make_pair(x,x); },options));
}

string to_db_type(const User*){ return "varchar(11)"; }

string to_db_type(const Datetime*){ return "datetime"; }

string show_input(DB db,string const& name,Date const& current){
	return "<br>"+name+":<input type=\"date\" name=\""+name+"\" value=\""+as_string(current)+"\">";
}

string to_db_type(const Date*){ return "date"; }

string show_input(DB db,string const& name,URL const& value){
	if(prefix("http",value.data)){
		return "<br><a href=\""+value.data+"\">"+name+":</a><input type=\"text\" name=\""+name+"\" value=\""+value.data+"\">";
	}
	return show_input(db,name,value.data);
}

#define E_PRINT(A) if(a==T::A) return o<<""#A;
#define E_LIST(A) v|=T::A;
#define E_PARSE(A) if(s==""#A) return T::A;
#define E_ESCAPED(A) values|=string{"'"#A "'"};

#define ENUM_DEFS(NAME,OPTIONS)\
	std::ostream& operator<<(std::ostream& o,NAME const& a){\
		OPTIONS(E_PRINT)\
		assert(0);\
	}\
	vector<NAME> options(const NAME*){\
		vector<NAME> v;\
		OPTIONS(E_LIST)\
		return v;\
	}\
	NAME rand(const NAME* a){\
		return choose(options(a));\
	}\
	NAME parse(const NAME*,string const& s){\
		OPTIONS(E_PARSE)\
		throw std::invalid_argument{""#NAME};\
	}\
	string show_input(DB db,string const& name,T const& value){\
		return drop_down(name,value,options(&value));\
	}\
	string escape(T const& a){ return "'"+as_string(a)+"'"; }\
	\
	string to_db_type(const T*){\
		vector<string> values;\
		OPTIONS(E_ESCAPED)\
		stringstream ss;\
		ss<<"enum(";\
		ss<<join(",",values);\
		ss<<")";\
		return ss.str();\
	}

#define T Part_state
ENUM_DEFS(Part_state,PART_STATES)
#undef T

#define T Machine
ENUM_DEFS(Machine,MACHINES)
#undef T

#define T Bend_type
ENUM_DEFS(Bend_type,BEND_TYPES)
#undef T

using Decimal=decimal::decimal32;

string to_db_type(const Decimal*){
	return "decimal(8,2)";
}

std::ostream& operator<<(std::ostream& o,Decimal const& a){
	return o<<decimal32_to_double(a);
}

Decimal rand(const Decimal*){ return (rand()%1000)/100; }

Decimal parse(const Decimal*,string const& s){
	auto sp=split('.',s);
	if(sp.size()==1){
		return std::decimal::make_decimal32((long long)stoi(sp[0]),0);
	}
	if(sp.size()==2){
		unsigned long long whole=stoi(sp[0]);
		int frac;
		if(sp[1].size()==1){
			frac=stoi(sp[1])*10;
		}else if(sp[1].size()==2){
			frac=stoi(sp[1]);
		}else{
			/*PRINT(s);
			PRINT(sp[1]);
			nyi*/
			throw "Max of 2 decimal places";
		}
		return std::decimal::make_decimal32(whole*100+frac,-2);
	}
	throw "Not a decimal";
}

string show_input(DB db,string const& name,Decimal value){
	return show_input(db,name,as_string(value))+" Max 2 decimal places";
}

string escape(Decimal a){ return as_string(a); }

vector<string> Material::suggestions()const{
	return {
		".090 Aluminum Plate",
		"1/8 Aluminum 6061",
		"1/8 Aluminum 5052",
		"Steel",
		"1/4 Aluminun Plate",
		"1x1 1/16 Tube",
		"1x2 1/16 Tube",
		"1x1 1/8 Tube",
		"1x2 1/8 Tube",
		"1/2 Hex",
		"3/8 Hex",
		"80/20",
		"Delrin",
	};
}

vector<string> Supplier::suggestions()const{
	return {"Vex","AndyMark","WCP","McMaster"};
}

string escape(Material const& s){
	return escape(s.s);
}

string show_input(DB db,string const& name,Subsystem_id const& current){
	auto q=query(
		db,
		"SELECT subsystem_id,name FROM subsystem_info WHERE (subsystem_id,edit_date) IN (SELECT subsystem_id,MAX(edit_date) FROM subsystem_info GROUP BY subsystem_id) AND valid"
	);

	auto m=mapf(
		[](auto row){
			assert(row.size()==2);
			return make_pair(*row[0],*row[1]);
		},
		q
	);

	//show the current one that it's part of, color coded
	//show the other current ones in a drop-down
	Subsystem_editor page;
	page.id=current;
	return drop_down(name,as_string(current),m)+link(page,"Current");
}

Dummy parse(const Dummy*,std::string const&){
	return Dummy{};
}

std::ostream& operator<<(std::ostream& o,Dummy){
	return o;
}
