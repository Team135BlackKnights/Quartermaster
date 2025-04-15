#include "home.h"
#include<algorithm>
#include<functional>
#include "export.h"

using namespace std;

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
	o<<"Content-type: text/html\n";
	o<<"Expires: 0\n\n";

	string name=heading+" - "+title_end();
	o<<html(
		head(
			title(name)
			/*+"<style>"
			"body{ background: grey; }"
			"</style>"*/
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
			"(SELECT MAX(id) FROM subsystem_info WHERE subsystem_id="+escape(id)+") "
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

string subsystem_name(DB db,optional<Subsystem_id> a){
	if(a) return subsystem_name(db,*a);
	return "Root";
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
			"(SELECT MAX(id) FROM part_info WHERE part_id="+escape(id)+") "
			"AND valid"
	);
	if(q.empty()) return "No part name found";
	//PRINT(q);
	assert(q.size()==1);
	return q[0];
}

tuple<string,string,URL> part_info(DB db,Part_id id){
	auto q=qm<optional<string>,optional<string>,optional<URL>>(
		db,
		"SELECT part_number,name,part_link "
		"FROM part_info "
		"WHERE (id) IN "
			"(SELECT MAX(id) FROM part_info WHERE part_id="+escape(id)+") "
			"AND valid"
	);
	if(q.empty()){
		return tuple<string,string,URL>{"?","No part name found",URL{""}};
	}
	//PRINT(q);
	assert(q.size()==1);
	auto row=q[0];
	string pn=get<0>(row)?*get<0>(row):string("?");
	string name=get<1>(row)?*get<1>(row):string("No part name found");
	URL url=get<2>(row)?*get<2>(row):URL{""};
	return make_tuple(pn,name,url);
}

string link(URL url,string body){
	stringstream ss;
	ss<<"<a href=\""<<url<<"\">"<<body<<"</a>";
	return ss.str();
}

string pretty_td(DB db, Part_id a){
	Part_editor page;
	page.id=a;
	auto info=part_info(db,a);
	return td(
		link(page,get<0>(info))+" "+
		[=](){
			auto url=get<2>(info);
			if(url.data.size()){
				return link(url,get<1>(info));
			}
			return get<1>(info);
		}()
	);
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

string highlight_color(Part_state state){
	//These all pastels to avoid obscuring text
	switch(state){
		case Part_state::in_design:
		case Part_state::need_prints:
			return "#ffcccc";//"red";
		case Part_state::build_list:
		case Part_state::need_to_cam:
			return "#ffcc44";//"orange";
		case Part_state::buy_list:
			return "#ffffaa";//"yellow";
		case Part_state::cut_list:
		case Part_state::find:
		case Part_state::_3d_print:
		case Part_state::fab:
			return "white";
		case Part_state::ordered:
			return "#ccccff";//"blue";
		case Part_state::fabbed:
		case Part_state::arrived:
		case Part_state::found:
			return "#ccffcc";//"green";
		default:
			PRINT(state);
			assert(0);
	}
}

string highlight_color(Assembly_state state){
	//These are all chosen from the same palette as for the part state highlighting.
	switch(state){
		case Assembly_state::in_design:
			return "#ffcccc";
		case Assembly_state::parts:
			return "#ccccff";
		case Assembly_state::assembly:
			return "white";
		case Assembly_state::done:
			return "#ccffcc";
		default:
			PRINT(state);
			assert(0);
	}
}

template<typename T>
string highlight_color(std::optional<T> const& a){
	if(a) return highlight_color(*a);
	return "#777777";//grey
}

template<typename T>
string key(string name,T const* a){
	stringstream ss;
	//ss<<h4(name);
	ss<<"<table>";
	for(auto state:options(a)){
		ss<<"<tr>";
		ss<<"<td bgcolor=\""+highlight_color(state)+"\">"<<state<<"</td>";
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

string td_top(string body){
	return "<td valign=top>"+body+"</td>";
}

string key(){
	stringstream ss;
	ss<<h3("Legend");
	ss<<"<table border>";
	ss<<tr(td("")+th("Assembly")+th("Part"));
	ss<<"<tr>";
	ss<<th("State colors");
	ss<<td(key("Assembly states",(Assembly_state*)0));
	ss<<td(key("Part states",(Part_state*)0));
	ss<<"</tr>";
	ss<<"<tr>";
	ss<<th("Intended workflow");
	ss<<td("<img src=\"/asm.png\" rel=\"Assembly workflow\">");
	ss<<td("<img src=\"/part.png\" rel=\"Part workflow\">");
	ss<<"</tr>";
	ss<<"</table>";
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
	auto data=qm<Subsystem_id,optional<string>,optional<string>,Assembly_state>(
		db,
		"SELECT subsystem_id,name,part_number,state "
		"FROM subsystem_info "
		"WHERE "
			"valid AND "
			"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) AND "
			"parent="+escape(id)
	);
	for(auto [subsystem_id,subsystem_name,part_number,state]:data){
		ss<<"<tr bgcolor=\""+highlight_color(state)+"\">";
		Subsystem_editor e;
		e.id=subsystem_id;
		ss<<td(link(e,part_number)+" "+as_string(subsystem_name));
		ss<<td(sub_table(db,subsystem_id,parents));
		ss<<"</tr>";
	}
	auto data2=qm<Part_id,optional<string>,optional<string>,unsigned,optional<Part_state>>(
		db,
		"SELECT part_id,name,part_number,qty,part_state "
		"FROM part_info "
		"WHERE "
			"valid AND "
			"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) AND "
			"subsystem="+escape(id)
	);
	for(auto [part_id,part_name,pn,qty,state]:data2){
		ss<<"<tr bgcolor=\""+highlight_color(state)+"\">";
		Part_editor e;
		e.id=part_id;
		ss<<td(link(e,pn)+" "+as_string(part_name))<<td(link(state))<<td(as_string(qty));
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

string part_tree(DB db){
	stringstream ss;
	ss<<h2("Current status");
	ss<<"<table border>";
	auto subs=qm<Subsystem_id,optional<string>,Assembly_state>(
		db,
		"SELECT subsystem_id,name,state "
		"FROM subsystem_info "
		"WHERE "
			"valid AND "
			"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) AND "
			"parent IS NULL"
	);
	for(auto [subsystem_id,subsystem_name,state]:subs){
		ss<<"<tr bgcolor=\""+highlight_color(state)+"\">";
		Subsystem_editor e;
		e.id=subsystem_id;
		ss<<td(link(e,subsystem_name))<<td(sub_table(db,subsystem_id,{}));
		ss<<"</tr>";
	}
	ss<<"</table>";
	ss<<key();
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

#define BOM_ITEM_ITEMS(X)\
	X(string,name)\
	X(string,supplier)\
	X(Part_number,part_number)\
	X(unsigned,qty)\
	X(Decimal,actual_cost)\
	X(Bom_category,category)\
	X(Decimal,official_cost)\
	X(Weight,weight)\
	X(vector<BOM_item>,children)
struct BOM_item{
	BOM_ITEM_ITEMS(INST)
};

BOM_item bom_data(DB db,optional<Subsystem_id> subsystem){
	auto asms=qm<optional<Subsystem_id>,Subsystem_id,string,Part_number,optional<bool>,optional<Weight>>(
		db,
		"SELECT parent,subsystem_id,name,part_number,dni,weight_override "
		"FROM subsystem_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) "
			"AND valid"
	);

	map<optional<Subsystem_id>,vector<Subsystem_id>> asm_by_parent;
	using Asm_info=tuple<string,Part_number,optional<bool>,optional<Weight>>;
	map<Subsystem_id,Asm_info> asm_by_id;
	for(auto elem:asms){
		asm_by_parent[get<0>(elem)]|=get<1>(elem);
		asm_by_id[get<1>(elem)]=make_tuple(get<2>(elem),get<3>(elem),get<4>(elem),get<5>(elem));
	}

	auto parts=qm<Subsystem_id,string,string,string,unsigned,bool,Decimal,Bom_exemption,Decimal,Weight>(
		db,
		"SELECT subsystem,name,part_supplier,part_number,qty,dni,price,bom_exemption,bom_cost_override,weight "
		"FROM part_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id)"
			" AND valid"
	);

	map<Subsystem_id,vector<BOM_item>> parts_by_parent;
	for(auto elem:parts){
		auto [subsystem,name,supplier,part_number,qty,dni,price,bom_exemption,bom_cost_override,weight]=elem;
		auto category=[=](){
			if(dni) return Bom_category::DNI;
			switch(bom_exemption){
				case Bom_exemption::none:
					if(price<5){
						return Bom_category::SUB_5D;
					}
					return Bom_category::STANDARD;
				case Bom_exemption::KOP:
					return Bom_category::KOP;
				case Bom_exemption::FIRST_Choice:
					return Bom_category::FIRST_Choice;
				default: assert(0);
			}
		}();
		auto official_cost=[=]()->Decimal{
			if(bom_cost_override!=0){
				return bom_cost_override;
			}
			if(category!=Bom_category::STANDARD) return 0;
			return qty*price;
		}();
		parts_by_parent[subsystem]|=BOM_item{
			name,
			supplier,
			part_number,
			dni?0:qty,
			price,
			category,
			official_cost,
			qty*weight
		};
	}

	std::function<BOM_item(optional<Subsystem_id>)> f;

	f=[&](optional<Subsystem_id> a)->BOM_item{
		BOM_item r;
		bool dni;
		optional<Weight> weight_override;
		if(a){
			auto x=asm_by_id[*a];
			r.name=get<0>(x);
			r.part_number=get<1>(x);
			dni=get<2>(x)?*get<2>(x):0;
			weight_override=get<3>(x);
		}else{
			r.name="Root";
			dni=0;
		}

		for(auto id:asm_by_parent[a]){
			r.children|=f(id);
		}
		if(a){
			r.children|=parts_by_parent[*a];
		}
		//now sum them up
		r.actual_cost=sum(mapf(
			[](auto x){
				return x.actual_cost;
			},
			r.children
		));
		if(dni){
			r.qty=0;
			r.category=Bom_category::DNI;
			r.official_cost=0;
			r.weight=Weight{0};
			return r;
		}else{
			r.qty=1;
			r.category=Bom_category::STANDARD;
			r.official_cost=sum(mapf([](auto x){ return x.official_cost; },r.children));
			r.weight=[=](){
				if(weight_override && weight_override>Weight{0}){
					return *weight_override;
				}
				return sum(mapf(
					[](auto x){ return x.weight; },
					r.children
				));
			}();
		}
		return r;
	};

	return f(subsystem);
}

template<typename Func,typename T>
void mapv(Func f,T t){
	for(auto const& elem:t){
		f(elem);
	}
}

string bom(DB db,optional<Subsystem_id> subsystem){
	stringstream ss;
	ss<<"<table border>";
	ss<<tr(join("",mapf(th,vector<string>{
		"Part name","Supplier","Part number","Qty","Actual cost","Rule Category","BOM cost","Weight"
	})));
	auto x=bom_data(db,subsystem);
	std::function<void(unsigned,BOM_item)> f;
	f=[&](unsigned indent,BOM_item const& b){
		ss<<"<tr>";
		ss<<td(indent_space(indent)+b.name);
		ss<<td(b.supplier);
		ss<<td(as_string(b.part_number));
		ss<<td(as_string(b.qty))<<td(as_string(b.actual_cost));
		ss<<td(as_string(b.category));
		ss<<td(as_string(b.official_cost));
		ss<<td(as_string(b.weight));
		ss<<"</tr>";
		//mapv(f,b.children);
		for(auto x:b.children){
			f(indent+1,x);
		}
	};
	f(0,x);
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

void inner(ostream& o,BOM const& a,DB db){
	make_page(
		o,
		subsystem_name(db,a.subsystem)+ " BOM",
		bom(db,a.subsystem)
	);
}

void inner(ostream& o,Home const& a,DB db){
	make_page(
		o,
		"Home",
		//parts_by_state(db,a)+
		part_tree(db)+
		indent_part_tree(db)//+bom(db,std::nullopt)
	);
}

std::string pretty_td(DB,Part_checkbox const& a){
        return td("<input type=\"checkbox\" name=\"part_checkbox\" value=\""+escape(a)+"\">");
}

std::string pretty_td(DB,Supplier const& a){
	By_supplier page;
	page.supplier=a;
	return td(link(page,as_string(a)));
}
