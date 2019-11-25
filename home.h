#ifndef HOME_H
#define HOME_H

#include "queries.h"
#include<algorithm>

void inner(std::ostream&,Home const&,DB);

struct Label{
	std::string text;
	std::optional<Request> url;

	Label(const char*);
	explicit Label(std::string);
	Label(std::string,Request);
};

void sortable_labels(std::ostream&,Request const&,std::vector<Label> const&);

template<typename T>
std::optional<T> parse(std::optional<T> const *t,const char *s){
	if(!s) return {};
	return parse(t,std::string{s});
}

template<typename T>
std::optional<T> parse(std::optional<T> const *x,std::optional<std::string> const& a){
	if(!a) return {};
	return parse(x,*a);
}

template<typename T>
T parse(T const* t,const char *s){
	if(!s) return {};
	return parse(t,std::string{s});
}

template<typename ... Ts>
std::vector<std::tuple<Ts...>> qm(DB db,std::string const& query_string){
	run_cmd(db,query_string);
	MYSQL_RES *result=mysql_store_result(db);
	if(result==NULL)nyi
	int fields=mysql_num_fields(result);
	using Tup=std::tuple<Ts...>;
	std::vector<Tup> r;
	MYSQL_ROW row;
	while((row=mysql_fetch_row(result))){
		Tup this_row;
		int i=0;
		std::apply(
			[&](auto&&... x){
				assert(i<fields);
				//assert(row[i]);
				((x=parse(&x,(row[i++]))), ...);
			},
			this_row
		);
		r|=this_row;
	}
	mysql_free_result(result);
	return r;
}

template<typename T>
std::string pretty_td(DB,T const& t){
	return td(as_string(t));
}

template<typename A,typename B>
std::string pretty_td(DB db,std::variant<A,B> const& a){
	if(std::holds_alternative<A>(a)){
		return pretty_td(db,std::get<A>(a));
	}
	return pretty_td(db,std::get<B>(a));
}

template<typename T>
std::string pretty_td(DB db,std::optional<T> const& a){
	if(a) return pretty_td(db,*a);
	return td("-");
}

std::string pretty_td(DB,Dummy);
std::string pretty_td(DB,URL const&);
std::string pretty_td(DB,Machine);
std::string pretty_td(DB,Part_state);
std::string pretty_td(DB,Part_id);
std::string pretty_td(DB,Subsystem_id);
std::string pretty_td(DB,User const&);


template<typename ... Ts>
void table_inner(std::ostream& o,DB db,Request const& page,std::vector<Label> const& labels,std::vector<std::tuple<Ts...>> const& a){
	sortable_labels(o,page,labels);
	
	std::vector<std::vector<std::pair<std::string,std::string>>> vv;
	for(auto row:a){
		std::vector<std::pair<std::string,std::string>> v;
		std::apply(
			[&](auto&&... x){
				( (v|=make_pair(as_string(x),pretty_td(db,x))), ... );
			},
			row
		);
		vv|=v;
	}

	std::optional<std::string> sort_by;
	std::visit([&](auto x){ sort_by=x.sort_by; },page);
	std::optional<std::string> sort_order;
	std::visit([&](auto x){ sort_order=x.sort_order; },page);
	
	std::optional<unsigned> index=[=]()->std::optional<unsigned>{
		for(auto [i,label]:enumerate(labels)){
			if(label.text==sort_by){
				return i;
			}
		}
		return {};
	}();

	bool desc=(sort_order=="desc");
	if(index){
		std::sort(
			begin(vv),
			end(vv),
			[index,desc](auto e1,auto e2){
				if(desc){
					return e1[*index]>e2[*index];
				}
				return e1[*index]<e2[*index];
			}
		);
	}

	for(auto row:vv){
		o<<"<tr>";
		for(auto [tag,elem]:row){
			(void)tag;
			o<<elem;
		}
		o<<"</tr>";
	}
}

template<typename ... Ts>
std::string as_table(DB db,Request const& page,std::vector<Label> const& labels,std::vector<std::tuple<Ts...>> const& a){
	std::stringstream ss;
	ss<<"<table border>";
	table_inner(ss,db,page,labels,a);
	ss<<"</table>";
	return ss.str();
}

void make_page(std::ostream& o,std::string const& heading,std::string const& main_body);

std::string indent_sub_table(DB db,unsigned indent,Subsystem_id id,std::set<Subsystem_id> parents);
std::string subsystem_name(DB,Subsystem_id);

#endif
