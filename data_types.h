#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include<iostream>
#include<vector>
#include<decimal/decimal>
#include "util.h"

std::string parse(const std::string*,std::string const&);
bool parse(const bool*,std::string const&);
unsigned parse(const unsigned*,std::string const&);
float parse(const float*,std::string const&);
double parse(const double*,std::string const&);

std::string with_suggestions(std::string const& name,std::string const& value,std::vector<std::string> const& suggestions);
std::string show_input(DB,std::string const& name,std::string const& value);
std::string show_input(DB,std::string const& name,int value);
std::string show_input(DB,std::string const& name,double value);
std::string show_input(DB,std::string const& name,unsigned value);
std::string show_input(DB,std::string const& name,bool value);
std::string to_db_type(const std::string*);
std::string to_db_type(bool const*);

std::string escape(std::string const&);

std::string escape(int);
int parse(const int*,std::string const&);
std::string to_db_type(const int*);
int rand(const int*);

using Decimal=std::decimal::decimal32;

std::ostream& operator<<(std::ostream&,Decimal const&);
Decimal rand(const Decimal*);
Decimal parse(const Decimal*,std::string const&);
std::string to_db_type(Decimal const *);
std::string show_input(DB db,std::string const& name,Decimal value);
std::string escape(Decimal a);

template<typename Sub,typename Data>
struct Wrap{
	Data data;
};

template<typename Sub,typename Data>
std::string to_db_type(const Wrap<Sub,Data>*){
	return to_db_type((Data*)0);
}

template<typename Sub,typename Data>
bool operator==(Wrap<Sub,Data> const& a,Wrap<Sub,Data> const& b){
	return a.data==b.data;
}

template<typename Sub,typename Data>
bool operator!=(Wrap<Sub,Data> const& a,Wrap<Sub,Data> const& b){
	return a.data!=b.data;
}

template<typename Sub,typename Data>
bool operator<(Wrap<Sub,Data> const& a,Wrap<Sub,Data> const& b){
	return a.data<b.data;
}

template<typename Sub,typename Data>
std::ostream& operator<<(std::ostream& o,Wrap<Sub,Data> const& a){
	return o<<a.data;
}

template<typename Sub,typename Data>
Sub rand(const Wrap<Sub,Data>*){
	return Sub{rand((Data*)0)};
}

template<typename Sub,typename Data>
Sub parse(const Wrap<Sub,Data>*,std::string const& s){
	return Sub{parse((Data*)0,s)};
}

template<typename Sub,typename Data>
std::string show_input(DB db,std::string const& name,Wrap<Sub,Data> const& current){
	return show_input(db,name,current.data);
}

template<typename Sub,typename Data>
std::string escape(Wrap<Sub,Data> const& a){
	return escape(a.data);
}

using Id=int;

struct Part_id:Wrap<Part_id,Id>{};
struct Meeting_id:Wrap<Meeting_id,Id>{};
struct Part_number:Wrap<Part_number,std::string>{};
std::string to_db_type(const Part_number*);

struct Subsystem_id:Wrap<Subsystem_id,Id>{};
std::string show_input(DB db,std::string const& name,Subsystem_id const& current);
std::string show_input(DB db,std::string const& name,std::optional<Subsystem_id> const& current);

struct User:Wrap<User,std::string>{};
std::string to_db_type(const User*);

struct Datetime:Wrap<Datetime,std::string>{};
std::string to_db_type(const Datetime*);

struct Date:Wrap<Date,std::string>{};
std::string show_input(DB db,std::string const& name,Date const& current);
std::string to_db_type(const Date*);

struct URL:Wrap<URL,std::string>{};
std::string show_input(DB db,std::string const& name,URL const& value);

#define NO_ADD(X) std::optional<X>& operator+=(std::optional<X>& a,std::optional<X> const&);
NO_ADD(Subsystem_id)
NO_ADD(Part_id)

#define PART_STATES(X)\
	X(in_design)\
	X(need_prints)\
	X(need_to_cam)\
	X(cut_list)\
	X(find)\
	X(found)\
	X(_3d_print)\
	X(fab)\
	X(fabbed)\
	X(buy_list)\
	X(ordered)\
	X(arrived)

#define MACHINES(X)\
	X(none)\
	X(by_hand)\
	X(velox)\
	X(shapeoko)\
	X(mill)\
	X(lathe)\
	X(cube_pro)\
	X(makerbot)\
	X(markforged)\
	X(laser)\
	X(find)

#define COMMA(A) A,

#define ENUM_DECL(NAME,OPTIONS)\
	enum class NAME{ OPTIONS(COMMA) };\
	std::ostream& operator<<(std::ostream&,NAME const&);\
	std::vector<NAME> options(const NAME*);\
	NAME rand(const NAME* a);\
	NAME parse(const NAME*,std::string const&);\
	std::string show_input(DB db,std::string const& name,NAME const& value);\
	std::string escape(NAME const&);\
	std::string to_db_type(const NAME*);\
	std::optional<NAME>& operator+=(std::optional<NAME>&,std::optional<NAME>);\

ENUM_DECL(Part_state,PART_STATES)
ENUM_DECL(Machine,MACHINES)

#define BEND_TYPES(X) X(none) X(easy_90) X(complex_not_90_or_mult)
ENUM_DECL(Bend_type,BEND_TYPES)

#define ASSEMBLY_STATES(X)\
	X(in_design)\
	X(parts)\
	X(assembly)\
	X(done)
ENUM_DECL(Assembly_state,ASSEMBLY_STATES)

#define EXPORT_ITEMS(X)\
	X(SUBSYSTEM)\
	X(SUBSYSTEM_INFO)\
	X(PART)\
	X(PART_INFO)\
	X(MEETING)\
	X(MEETING_INFO)
ENUM_DECL(Export_item,EXPORT_ITEMS)

#define BOM_EXEMPTION_OPTIONS(X)\
	X(none)\
	X(KOP)\
	X(FIRST_Choice)
ENUM_DECL(Bom_exemption,BOM_EXEMPTION_OPTIONS)

#define BOM_CATEGORY_OPTIONS(X)\
	X(STANDARD)\
	X(DNI)\
	X(SUB_5D)\
	X(KOP)\
	X(FIRST_Choice)
ENUM_DECL(Bom_category,BOM_CATEGORY_OPTIONS)

template<typename T>
struct Suggest{
	std::string s;

	Suggest(){}
	Suggest(std::string s1):s(std::move(s1)){}
	Suggest(T);
	virtual std::vector<std::string> suggestions()const=0;
};

template<typename T>
bool operator==(Suggest<T> const& a,Suggest<T> const& b){
	return a.s==b.s;
}

template<typename T>
bool operator!=(Suggest<T> const& a,Suggest<T> const& b){
	return !(a==b);
}

template<typename T>
std::ostream& operator<<(std::ostream& o,Suggest<T> const& a){
	return o<<a.s;
}

template<typename T>
T rand(const Suggest<T>*){
	T r;
	r.s=rand((std::string*)0);
	return r;
}

template<typename T>
std::string to_db_type(const Suggest<T>*){
	return "varchar(20)";
}

template<typename T>
T parse(const Suggest<T>*,std::string s){
	T r;
	r.s=move(s);
	return r;
}

template<typename T>
std::string show_input(DB db,std::string const& name,Suggest<T> const& value){
	return with_suggestions(name,value.s,value.suggestions());
}

template<typename T>
std::string escape(Suggest<T> const& s){
	return escape(s.s);
}

struct Supplier:public Suggest<Supplier>{
	std::vector<std::string> suggestions()const;
};

struct Material:public Suggest<Material>{
	std::vector<std::string> suggestions()const;
};

template<typename T>
std::string to_db_type(const std::optional<T>*){
	return to_db_type((T*)nullptr);
}

template<typename T>
std::optional<T> rand(const std::optional<T>*){
	if(rand()%2) return rand((T*)0);
	return std::nullopt;
}

template<typename T>
std::optional<T> parse(const std::optional<T>*,std::string const& s){
	try{
		return parse((T*)0,s);
	}catch(...){
		return std::nullopt;
	}
}

template<typename T>
std::string show_input(DB db,std::string const& name,std::optional<T> const& a){
	if(a) return show_input(db,name,*a);
	return show_input(db,name,T{});
}

template<typename T>
std::string escape(std::optional<T> const& a){
	if(a) return escape(*a);
	return "NULL";
}

struct Dummy{};
Dummy parse(const Dummy*,std::string const&);
std::ostream& operator<<(std::ostream&,Dummy);

class Subsystem_prefix{
	char a,b;

	public:
	Subsystem_prefix();
	Subsystem_prefix(char,char);

	std::string get()const;	
};
std::string to_db_type(const Subsystem_prefix*);
Subsystem_prefix parse(Subsystem_prefix const*,std::string const&);
std::string show_input(DB,std::string const&,Subsystem_prefix const&);
std::string escape(Subsystem_prefix const&);
bool operator==(Subsystem_prefix const&,Subsystem_prefix const&);
bool operator!=(Subsystem_prefix const&,Subsystem_prefix const&);
bool operator<(Subsystem_prefix const&,Subsystem_prefix const&);
bool operator>(Subsystem_prefix const&,Subsystem_prefix const&);
std::ostream& operator<<(std::ostream&,Subsystem_prefix const&);
Subsystem_prefix rand(Subsystem_prefix const*);

class Three_digit{
	int value;//000-999 only.

	public:
	Three_digit();
	explicit Three_digit(int);
	Three_digit& operator=(int);
	operator int()const;
};
std::ostream& operator<<(std::ostream& o,Three_digit);

struct Part_number_local{
	//Should look something like:
	//XX000-1425-2019
	Subsystem_prefix subsystem_prefix;
	Three_digit num;

	explicit Part_number_local(std::string const&);
	explicit Part_number_local(Part_number const&);
	Part_number_local(Subsystem_prefix,Three_digit);

	std::string get()const;
};

std::ostream& operator<<(std::ostream&,Part_number_local const&);
bool operator<(Part_number_local const&,Part_number_local const&);
std::string escape(Part_number_local const&);
Part_number_local next(Part_number_local);

struct Part_checkbox:Wrap<Part_checkbox,Part_id>{};
std::string show_input(DB,std::string const&,Part_checkbox const&);

struct Weight:Wrap<Weight,Decimal>{};

#endif
