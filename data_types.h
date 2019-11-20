#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include<iostream>
#include<vector>
#include<decimal/decimal>
#include "util.h"

std::string parse(const std::string*,std::string);

std::string with_suggestions(std::string name,std::string value,std::vector<std::string> const& suggestions);
std::string show_input(DB,std::string name,std::string value);
std::string show_input(DB,std::string name,int value);
std::string show_input(DB,std::string name,double value);
std::string show_input(DB,std::string name,unsigned value);
std::string show_input(DB,std::string name,bool value);

std::string escape(std::string);

std::string escape(int);
int parse(const int*,std::string s);
std::string to_db_type(const int*);
int rand(const int*);

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
std::ostream& operator<<(std::ostream& o,Wrap<Sub,Data> const& a){
	return o<<a.data;
}

template<typename Sub,typename Data>
Sub rand(const Wrap<Sub,Data>*){
	return Sub{rand((Data*)0)};
}

template<typename Sub,typename Data>
Sub parse(const Wrap<Sub,Data>*,std::string s){
	return Sub{parse((Data*)0,s)};
}

template<typename Sub,typename Data>
std::string show_input(DB db,std::string name,Wrap<Sub,Data> const& current){
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
std::string show_input(DB db,std::string name,Subsystem_id const& current);

struct User:Wrap<User,std::string>{};
std::string to_db_type(const User*);

struct Datetime{
	std::string s;
};

std::ostream& operator<<(std::ostream&,Datetime const&);
std::string to_db_type(const Datetime*);
Datetime parse(const Datetime*,std::string const&);

struct Date{
	//not just a typedef so that can dispatch on the type
	std::string s;
};

bool operator==(Date,Date);
bool operator!=(Date,Date);

std::ostream& operator<<(std::ostream&,Date);

Date rand(const Date*);
Date parse(const Date*,std::string);

std::string show_input(DB db,std::string name,Date current);

std::string escape(Date);
std::string to_db_type(const Date*);

struct URL{
	std::string s;
};

bool operator==(URL,URL);
bool operator!=(URL,URL);

std::ostream& operator<<(std::ostream&,URL const&);

URL rand(const URL*);
URL parse(const URL*,std::string const&);

std::string to_db_type(const URL*);
std::string escape(URL const&);

std::string show_input(DB db,std::string name,URL const& value);

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
	std::string show_input(DB db,std::string name,NAME value);\
	std::string escape(NAME a);\
	std::string to_db_type(const NAME*);\

ENUM_DECL(Part_state,PART_STATES)
ENUM_DECL(Machine,MACHINES)

#define BEND_TYPES(X) X(none) X(easy_90) X(complex_not_90_or_mult)
ENUM_DECL(Bend_type,BEND_TYPES)

using Decimal=std::decimal::decimal32;

std::ostream& operator<<(std::ostream&,Decimal const&);
Decimal rand(const Decimal*);
Decimal parse(const Decimal*,std::string);
std::string to_db_type(const Decimal*);
std::string show_input(DB db,std::string name,Decimal value);
std::string escape(Decimal a);

template<typename T>
struct Suggest{
	std::string s;

	Suggest(){}
	Suggest(std::string s1):s(s1){}
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
	r.s=s;
	return r;
}

template<typename T>
std::string show_input(DB db,std::string name,Suggest<T> const& value){
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
T rand(const std::optional<T>*){ return rand((T*)0); }

template<typename T>
T parse(const std::optional<T>*,std::string s){
	if(s=="") return {};
	return parse((T*)0,s);
}

template<typename T>
std::string show_input(DB db,std::string name,std::optional<T> const& a){
	if(a) return show_input(db,name,*a);
	return show_input(db,name,"");
}

template<typename T>
std::string escape(std::optional<T> const& a){
	if(a) return escape(*a);
	return "NULL";
}

struct Dummy{};
Dummy parse(const Dummy*,std::string const&);
std::ostream& operator<<(std::ostream&,Dummy);

#endif
