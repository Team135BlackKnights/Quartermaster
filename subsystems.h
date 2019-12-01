#ifndef SUBSYSTEMS_H
#define SUBSYSTEMS_H

#include "queries.h"
#include "home.h"

void inner(std::ostream&,Subsystems const&,DB);

std::string make_table(Request const& page,std::vector<std::string> const& columns,std::vector<std::vector<std::optional<std::string>>>);

template<typename A,typename B>
std::variant<A,B> parse(const std::variant<A,B>*,std::string const& s){
    //obviously, this is not fully general.
	return parse((A*)0,s);
}

template<typename ... Ts>
std::string table_with_totals(DB db,Request const& page,std::vector<Label> const& labels,std::vector<std::tuple<Ts...>> const& a){
	std::stringstream ss;
	ss<<"<table border>";
	table_inner(ss,db,page,labels,a);
	if(!a.empty()){
		ss<<"<tr>";
		auto t=sum(a);
		std::apply([&](auto&&... x){ ((ss<<pretty_td(db,x)),...); },t);
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

template<typename ... Ts>
std::tuple<Ts...>& operator+=(std::tuple<Ts...>& a,std::tuple<Ts...> const& b){
	std::apply(
		[&](auto&&... x){
			//Do a bunch of nasty pointer arithmetic to figure out where the parallel item is in "b"
			(( x+=*(typename std::remove_reference<decltype(x)>::type*)((char*)&b+((char*)&x-(char*)&a)) ), ... );
		},
		a
	);
	return a;
}

Dummy& operator+=(Dummy&,Dummy);

template<typename T>
std::variant<T,std::string>& operator+=(std::variant<T,std::string>& a,std::variant<T,std::string> const&){
	a="Total";
	return a;
}

std::string show_table(DB db,Request const& page,Table_name const& name,std::optional<std::string> title={});
std::string subsystem_state_count(DB,Request const&);
std::string subsystem_machine_count(DB,Request const&);

#define A_COMMA(A,B) A,
#define B_STR_COMMA(A,B) ""#B ","
#define LABEL_B(A,B) labels|=Label{""#B};

#define HISTORY_TABLE(NAME,ITEMS)\
std::string history_##NAME(DB db,Request const& page){\
	auto q=qm<\
		ITEMS(A_COMMA)\
		Dummy\
	>(\
		db,\
		"SELECT "\
		ITEMS(B_STR_COMMA)\
		"0 FROM "#NAME "_info ORDER BY id"\
	);\
	vector<Label> labels;\
	ITEMS(LABEL_B)\
	return h2("History")+as_table(db,page,labels,q);\
}

#endif
