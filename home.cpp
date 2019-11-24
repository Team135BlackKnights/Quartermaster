#include "home.h"
#include<algorithm>
#include "export.h"

using namespace std;

template<typename T>
vector<T> convert1(vector<vector<optional<string>>> const& in){
	return mapf(
		[](auto x){
			assert(x.size()==1);
			if(!x[0]) return T{};
			assert(x[0]);
			return parse((T*)0,*x[0]);
		},
		in
	);
}

template<typename T>
vector<T> q1(DB db,string const& query_string){
	auto q=query(db,query_string);
	return convert1<T>(q);
}

string title_end(){ return "1425 Parts System"; }

string nav(){
	return ""
		#define X(A) +[]()->string{ \
			return link(A{},""#A)+" "; \
		}()
		BASIC_PAGES(X)
		#undef X
	+"<br>"
	;
}

void make_page(std::ostream& o,string const& heading,string const& main_body){
	o<<"Content-type: text/html\n\n";

	string name=heading+" - "+title_end();
	o<<html(
		head(
			title(name)
		)+
		body(
			h1(heading)+nav()+main_body
		)
	);

}

string pretty_td(DB,Dummy){
	return "";
}

string pretty_td(DB,URL const& a){
	return td("<a href=\""+a.data+"\">"+a.data+"</a>");
}

string pretty_td(DB,Machine a){
	Machine_page m;
	m.machine=a;
	return td(link(m,as_string(a)));
}

string pretty_td(DB,Part_state a){
	State page;
	page.state=a;
	return td(link(page,as_string(a)));
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
	Subsystem_editor page;
	page.id=a;
	return td(link(page,subsystem_name(db,a)));
}

string part_name(DB db,Part_id id){
	auto q=q1<string>(
		db,
		"SELECT name "
		"FROM part_info "
		"WHERE (id) IN "
			"(SELECT MAX(id) FROM part_info WHERE part_id="+as_string(id)+") "
			"AND valid"
	);
	//if(q.empty()) return "what?";
	//PRINT(q);
	assert(q.size()==1);
	return q[0];
}

string pretty_td(DB db, Part_id a){
	Part_editor page;
	page.id=a;
	return td(link(page,part_name(db,a)));
}

string pretty_td(DB,User const& a){
	By_user page;
	page.user=a;
	return td(link(page,as_string(a)));
}

Label::Label(const char *s):text(s){}
Label::Label(std::string s):text(move(s)){}
Label::Label(string s,Request r):text(std::move(s)),url(std::move(r)){}

std::ostream& operator<<(std::ostream& o,Label const& a){
	if(a.url) return o<<link(*a.url,a.text);
	return o<<a.text;
}

void sortable_labels(ostream& o,Request const& page,vector<Label> const& labels){
	o<<"<tr>";
	for(auto label:labels){
		o<<"<th>";
		auto p2=page;
		std::visit([&](auto &x){ x.sort_by=label.text; x.sort_order="asc"; }, p2);
		auto p3=page;
		std::visit([&](auto &x){ x.sort_by=label.text; x.sort_order="desc"; }, p3);
		o<<label<<" ";
		o<<link(p2,"/\\");
		o<<" "<<link(p3,"\\/");
		o<<"</th>";
	}
	o<<"</tr>";
}

string link(Part_state a){
	State r;
	r.state=a;
	return link(r,as_string(a));
}

template<typename T>
string link(optional<T> a){
	if(a) return link(*a);
	return "NULL";
}

string link(Request const& r,optional<string> s){
	if(s) return link(r,*s);
	return link(r,string{"NULL"});
}

string link(Request const& r,const char *s){
	return link(r,s?string(s):"NULL");
}

string link(optional<string> const& url,string const& text){
	if(!url) return text;
	stringstream ss;
	ss<<"<a href=\""<<*url<<"\">"<<text<<"</a>";
	return ss.str();
}

string sub_table(DB db,Subsystem_id id,set<Subsystem_id> parents){
	if(parents.count(id)){
		stringstream ss;
		ss<<"Loop!"<<id<<" "<<parents;
		return ss.str();
	}
	parents|=id;
	stringstream ss;
	ss<<"<table border>";
	auto data=qm<Subsystem_id,optional<string>>(
		db,
		"SELECT subsystem_id,name "
		"FROM subsystem_info "
		"WHERE "
			"valid AND "
			"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) AND "
			"parent="+escape(id)
	);
	for(auto [subsystem_id,subsystem_name]:data){
		ss<<"<tr>";
		Subsystem_editor e;
		e.id=subsystem_id;
		ss<<td(link(e,subsystem_name))<<td(sub_table(db,subsystem_id,parents));
		ss<<"</tr>";
	}
	auto data2=qm<Part_id,optional<string>,optional<Part_state>>(
		db,
		"SELECT part_id,name,part_state "
		"FROM part_info "
		"WHERE "
			"valid AND "
			"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) AND "
			"subsystem="+escape(id)
	);
	for(auto [part_id,part_name,state]:data2){
		ss<<"<tr>";
		Part_editor e;
		e.id=part_id;
		ss<<td(link(e,part_name))<<td(link(state));
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

string part_tree(DB db){
	stringstream ss;
	ss<<h2("Part tree");
	ss<<"<table border>";
	auto subs=qm<Subsystem_id,optional<string>>(
		db,
		"SELECT subsystem_id,name "
		"FROM subsystem_info "
		"WHERE "
			"valid AND "
			"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) AND "
			"parent IS NULL"
	);
	for(auto [subsystem_id,subsystem_name]:subs){
		ss<<"<tr>";
		Subsystem_editor e;
		e.id=subsystem_id;
		ss<<td(link(e,subsystem_name))<<td(sub_table(db,subsystem_id,{}));
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}
string indent_space(unsigned indent){
	stringstream ss;
	for(auto _:range(indent*4)){
		(void)_;
		ss<<"&nbsp;";
	}
	return ss.str();
}

string indent_sub_table(DB db,unsigned indent,Subsystem_id id,set<Subsystem_id> parents){
	if(parents.count(id)){
		stringstream ss;
		ss<<"Loop!"<<id<<" "<<parents;
		return ss.str();
	}
	parents|=id;
	stringstream ss;
	auto data=qm<Subsystem_id,optional<string>>(
		db,
		"SELECT subsystem_id,name "
		"FROM subsystem_info "
		"WHERE "
			"valid AND "
			"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) AND "
			"parent="+escape(id)
	);
	for(auto [subsystem_id,subsystem_name]:data){
		ss<<"<tr>";
		Subsystem_editor e;
		e.id=subsystem_id;
		ss<<td(indent_space(indent)+link(e,subsystem_name));
		Subsystem_new new_sub;
		new_sub.parent=subsystem_id;
		Part_new new_part;
		new_part.subsystem=subsystem_id;
                ss<<td(link(new_sub,"New subsystem")+" "+link(new_part,"New part"));
		ss<<"</tr>";
		ss<<indent_sub_table(db,indent+1,subsystem_id,parents);
	}
	auto data2=qm<Part_id,string,Part_state>(
		db,
		"SELECT part_id,name,part_state "
		"FROM part_info "
		"WHERE "
			"valid AND "
			"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) AND "
			"subsystem="+escape(id)
	);
	for(auto [part_id,part_name,state]:data2){
		ss<<"<tr>";
		Part_editor e;
		e.id=part_id;
		ss<<td(indent_space(indent)+link(e,part_name))<<td(link(state));
		ss<<"</tr>";
	}
	return ss.str();
}

string indent_part_tree(DB db){
	stringstream ss;
	ss<<h2("BOM-style tree");
	ss<<"<table border>";
	ss<<tr(th("Name")+th("State"));
	auto subs=qm<Subsystem_id,optional<string>>(
		db,
		"SELECT subsystem_id,name "
		"FROM subsystem_info "
		"WHERE "
			"valid AND "
			"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) AND "
			"parent IS NULL"
	);
	for(auto [subsystem_id,subsystem_name]:subs){
		ss<<"<tr>";
		Subsystem_editor e;
		e.id=subsystem_id;
		ss<<td(link(e,subsystem_name));
		Subsystem_new new_sub;
		new_sub.parent=subsystem_id;
		Part_new new_part;
		new_part.subsystem=subsystem_id;
		ss<<td(link(new_sub,"New subsystem")+" "+link(new_part,"New part"));
		ss<<"</tr>";
		ss<<indent_sub_table(db,1,subsystem_id,{});
	}
	ss<<"</table>";
	return ss.str();
}

string parts_by_state(DB db,Request const& page){
	auto a=qm<optional<Part_state>,Subsystem_id,Part_id>(
		db,
		"SELECT part_state,subsystem,part_id "
		"FROM part_info "
		"WHERE "
			"valid "
			"AND id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
		"ORDER BY part_state,subsystem "
	);
	return h2("Parts by state")+as_table(db,page,vector<Label>{"State","Subsystem","Part"},a);
}

void inner(ostream& o,Home const& a,DB db){
	make_page(
		o,
		"Home",
		parts_by_state(db,a)+
		part_tree(db)+
		indent_part_tree(db)
	);
}

