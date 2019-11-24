#include<algorithm>
#include "util.h"
#include "data_types.h"
#include "auth.h"
#include "queries.h"

using namespace std;

//TODO: Move to util
template<typename Func,typename T>
vector<T> filter(Func f,vector<T> const& v){
	vector<T> r;
	for(auto const& elem:v){
		if(f(elem)) r|=elem;
	}
	return r;
}

/*Subsystem_id& operator+=(Subsystem_id& a,Subsystem_id b){
	a.data=0;//obviously invalid; real ones start numbering at 1.
	return a;
}*/

template<typename T>
optional<T>& operator+=(optional<T>& a,optional<T> const& b){
	if(b) a={};
	return a;
}

template<typename T>
variant<T,string>& operator+=(std::variant<T,string>& a,std::variant<T,string> const&){
	a="Total";
	return a;
}

Dummy& operator+=(Dummy& a,Dummy){
	return a;
}

template<typename ... Ts>
tuple<Ts...>& operator+=(tuple<Ts...>& a,tuple<Ts...> const& b){
	std::apply(
		[&](auto&&... x){
			//Do a bunch of nasty pointer arithmetic to figure out where the parallel item is in "b"
			(( x+=*(typename std::remove_reference<decltype(x)>::type*)((char*)&b+((char*)&x-(char*)&a)) ), ... );
		},
		a
	);
	return a;
}

template<typename T>
T sum(vector<T> const& v){
	T r{};
	for(auto elem:v){
		r+=elem;
	}
	return r;
}

template<typename ... Ts>
std::ostream& operator<<(std::ostream& o,std::tuple<Ts...> const& a){
	o<<"( ";
	std::apply([&](auto&&... x){ ((o<<x<<" "), ...); },a);
	return o<<")";
}

template<typename T>
vector<T> convert1(vector<vector<optional<string>>> const& in){
	return mapf(
		[](auto x){
			assert(x.size()==1);
			return parse((T*)0,*x[0]);
		},
		in
	);
}

template<typename ... Ts>
tuple<Ts...> convert_row(vector<optional<string>> const& row){
	tuple<Ts...> t;
	size_t i=0;
	std::apply([&](auto&&... x){ ((x=parse(&x,*row[i++])), ...); },t);
	return t;
}

template<typename ... Ts>
vector<tuple<Ts...>> convert(vector<vector<optional<string>>> const& in){
	return mapf(convert_row<Ts...>,in);
}

template<typename T>
vector<T> q1(DB db,string const& query_string){
	auto q=query(db,query_string);
	//PRINT(q);
	return convert1<T>(q);
}

template<typename T>
T parse(T const* t,const char *s){
	if(!s) throw "Unexpected NULL";
	return parse(t,string{s});
}

template<typename T>
optional<T> parse(std::optional<T> const *t,const char *s){
	if(!s) return {};
	return parse(t,string{s});
}

template<typename ... Ts>
vector<tuple<Ts...>> qm(DB db,std::string const& query_string){
	run_cmd(db,query_string);
	MYSQL_RES *result=mysql_store_result(db);
	if(result==NULL)nyi
	int fields=mysql_num_fields(result);
	using Tup=tuple<Ts...>;
	vector<Tup> r;
	MYSQL_ROW row;
	while((row=mysql_fetch_row(result))){
		Tup this_row;
		int i=0;
		std::apply(
			[&](auto&&... x){
				assert(i<fields);
				assert(row[i]);
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
string pretty_td(DB,T const& t){
	return td(as_string(t));
}

template<typename A,typename B>
string pretty_td(DB db,std::variant<A,B> const& a){
	if(holds_alternative<A>(a)){
		return pretty_td(db,get<A>(a));
	}
	return pretty_td(db,get<B>(a));
}

template<typename T>
string pretty_td(DB db,std::optional<T> const& a){
	if(a) return pretty_td(db,*a);
	return td("-");
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

string link(optional<string> const& url,string const& text){
	if(!url) return text;
	stringstream ss;
	ss<<"<a href=\""<<*url<<"\">"<<text<<"</a>";
	return ss.str();
}

struct Label{
	string text;
	optional<Request> url;

	Label(const char *s):text(s){}
	explicit Label(std::string s):text(move(s)){}
	Label(string s,Request r):text(std::move(s)),url(std::move(r)){}
};

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

template<typename T>
vector<pair<size_t,T>> enumerate(std::vector<T> const& a){
	vector<pair<size_t,T>> r;
	for(size_t i=0;i<a.size();i++){
		r|=make_pair(i,a[i]);
	}
	return r;
}

template<typename ... Ts>
void table_inner(ostream& o,DB db,Request const& page,vector<Label> const& labels,vector<tuple<Ts...>> const& a){
	sortable_labels(o,page,labels);
	
	vector<vector<pair<string,string>>> vv;
	for(auto row:a){
		vector<pair<string,string>> v;
		std::apply(
			[&](auto&&... x){
				( (v|=make_pair(as_string(x),pretty_td(db,x))), ... );
			},
			row
		);
		vv|=v;
	}

	optional<string> sort_by;
	std::visit([&](auto x){ sort_by=x.sort_by; },page);
	optional<string> sort_order;
	std::visit([&](auto x){ sort_order=x.sort_order; },page);
	
	optional<unsigned> index=[=]()->optional<unsigned>{
		for(auto [i,label]:enumerate(labels)){
			if(label.text==sort_by){
				return i;
			}
		}
		return {};
	}();

	bool desc=(sort_order=="desc");
	if(index){
		sort(
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
string as_table(DB db,Request const& page,vector<Label> const& labels,vector<tuple<Ts...>> const& a){
	stringstream ss;
	ss<<"<table border>";
	table_inner(ss,db,page,labels,a);
	ss<<"</table>";
	return ss.str();
}

template<typename ... Ts>
string table_with_totals(DB db,Request const& page,vector<Label> const& labels,vector<tuple<Ts...>> const& a){
	stringstream ss;
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

string as_table(vector<string> const& labels,vector<vector<std::string>> const& in){
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

string as_table(vector<string> const& labels,vector<vector<std::optional<string>>> const& in){
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
			return link(A{},""#A)+" "; \
		}()
		BASIC_PAGES(X)
		#undef X
	+"<br>"
	;
}

void make_page(std::ostream& o,string const& heading,string const& main_body){
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

string parts_by_state(DB db,Request const& page){
	auto a=qm<Part_state,Subsystem_id,Part_id>(
		db,
		"SELECT part_state,subsystem,part_id "
		"FROM part_info "
		"WHERE "
			"valid "
			"AND id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
		"ORDER BY part_state,subsystem "
	);
	return as_table(db,page,vector<Label>{"State","Subsystem","Part"},a);
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
	auto data=qm<Subsystem_id,string>(
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
		ss<<td(link(e,part_name))<<td(as_string(state));
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

string part_tree(DB db){
	stringstream ss;
	ss<<h2("Part tree");
	ss<<"<table border>";
	auto subs=qm<Subsystem_id,string>(
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

void inner(ostream& o,Home const& a,DB db){
	make_page(o,"Home",parts_by_state(db,a)+part_tree(db));
}

string make_table(Request const& page,vector<string> const& columns,vector<vector<optional<string>>> q){
	stringstream ss;
	ss<<"<table border>";
	sortable_labels(ss,page,mapf([](auto a){ return Label(a); },columns));
	ss<<"<tr>";

	optional<string> sort_by;
	std::visit([&sort_by](auto x){ sort_by=x.sort_by; },page);
	optional<int> sort_index=[&]()->optional<int>{
		for(auto [i,name]:enumerate(columns)){
			if(name==sort_by) return i;
		}
		return {};
	}();
	bool desc=[=](){
		std::optional<std::string> sort_order;
		std::visit([&](auto x){ sort_order=x.sort_order; },page);
		if(sort_order=="desc") return 1;
		return 0;
	}();

	if(sort_index){
		sort(
			begin(q),
			end(q),
			[sort_index,desc](auto const& e1,auto const& e2){
				if(desc) return e1[*sort_index]>e2[*sort_index];
				return e1[*sort_index]<e2[*sort_index];
			}
		);
	}
	for(auto row:q){
		ss<<"<tr>";
		for(auto elem:row){
			ss<<td(as_string(elem));
		}
		ss<<"</tr>";
	}
	ss<<"</tr>";
	ss<<"</table>";
	return ss.str();
}

string make_table(Request const& page,vector<vector<optional<string>>> const& a){
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

string show_table(DB db,Request const& page,Table_name const& name,optional<string> title={}){
	auto columns=mapf([](auto x){ return *x; },firsts(query(db,"DESCRIBE "+name)));
	stringstream ss;
	ss<<h2(title?*title:name);
	auto q=query(db,"SELECT * FROM "+name);
	ss<<make_table(page,columns,q);
	return ss.str();
}

vector<int> get_ids(DB db,Table_name const& table){
	vector<int> r;
	for(auto row:query(db,"SELECT id FROM "+table)){
		r|=stoi(*row.at(0));
	}
	return r;
}

template<typename A,typename B>
std::variant<A,B> parse(const variant<A,B>*,string const& s){
	return parse((A*)0,s);
}

State make_state(Part_state const& a){
	State r;
	r.state=a;
	return r;
}

string subsystem_state_count(DB db,Request const& page){
	auto q=qm<
		variant<Subsystem_id,string>,
		#define X(A) int,
		PART_STATES(X)
		#undef X
		unsigned
	>(
		db,
                "SELECT subsystem_id,"
		#define X(A) "SUM(part_info.part_state='"#A "'),"
		PART_STATES(X)
		#undef X
		"COUNT(*) "
		"FROM subsystem_info,part_info "
		"WHERE "
			"subsystem_info.id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) "
			"AND subsystem_info.valid "
			"AND part_info.valid "
			"AND part_info.id IN (SELECT max(id) FROM part_info GROUP BY part_id) "
			"AND part_info.subsystem=subsystem_info.subsystem_id "
		"GROUP BY subsystem_id"
	);
	return h2("Number of parts by state")+table_with_totals(
		db,
		page,
		vector<Label>{
			Label{"Subsystem"}
			#define X(A) ,Label{""#A,make_state(Part_state::A)}
			PART_STATES(X)
			#undef X
			,Label{"Total"}
		},
		q
	);
}

Machine_page make_machine_page(Machine a){
	Machine_page r;
	r.machine=a;
	return r;
}

string subsystem_machine_count(DB db,Request const& page){
	auto q=qm<
		variant<Subsystem_id,string>,
		#define X(A) int,
		MACHINES(X)
		#undef X
		Dummy
	>(
		db,
                "SELECT subsystem_id,"
		#define X(A) "SUM(part_info.machine='"#A "'),"
		MACHINES(X)
		#undef X
		"0 "
		"FROM subsystem_info,part_info "
		"WHERE "
			"subsystem_info.id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) "
			"AND subsystem_info.valid "
			"AND part_info.valid "
			"AND part_info.id IN (SELECT max(id) FROM part_info GROUP BY part_id) "
			"AND part_info.subsystem=subsystem_info.subsystem_id "
		"GROUP BY subsystem_id"
	);
	return h2("Number of parts by machine")+"Note: This is regardless of state, so includes finished parts."+table_with_totals(
		db,
		page,
		{
			Label{"Subsystem"}
			#define X(A) ,Label{""#A,make_machine_page(Machine::A)}
			MACHINES(X)
			#undef X
		},
		q
	);
}

string show_current_subsystems(DB db,Request const& page){
		/*+
		h2("Debug info")+
		[=](){
			stringstream ss;
			for(auto id:get_ids(db,"subsystem")){
				ss<<link(Subsystem_editor{id},"Edit "+as_string(id));
			}
			return ss.str();
		}()+
		show_table(db,"subsystem")+*/

	return as_table(
		db,
		page,
		vector<Label>{"Name","Prefix","Parent"},
		qm<Subsystem_id,Subsystem_prefix,optional<Subsystem_id>>(
			db,
			"SELECT subsystem_id,prefix,parent "
			"FROM subsystem_info "
			"WHERE "
				"valid AND "
				"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id)"
		)
	);
}

void inner(ostream& o,Subsystems const& a,DB db){
	return make_page(
		o,
		"Subsystems",
		show_current_subsystems(db,a)
		+subsystem_state_count(db,a)
		+subsystem_machine_count(db,a)
		+show_table(db,a,"subsystem_info","History")
	);
}

template<typename T>
string redirect_to(T const& t){
	stringstream ss;
	//ss<<"<meta http-equiv = \"refresh\" content = \"2; url = ";
	ss<<"<meta http-equiv = \"refresh\" content = \"0; url = ";
	ss<<"?"<<to_query(t);
	ss<<"\" />";
	ss<<"You should have been automatically redirected to:"<<t<<"\n";
	return ss.str();
}

template<typename T>
void inner_new(ostream& o,DB db,Table_name const& table){
	run_cmd(db,"INSERT INTO "+table+" VALUES ()");
	auto q=query(db,"SELECT LAST_INSERT_ID()");
	//PRINT(q);
	assert(q.size()==1);
	assert(q[0].size()==1);
	assert(q[0][0]);
	auto id=stoi(*q[0][0]);

	T page;
	page.id={id};
	make_page(
		o,
		"Subsystem new",
		redirect_to(page)
	);
}

void inner(ostream& o,Subsystem_new const&,DB db){
	inner_new<Subsystem_editor>(o,db,"subsystem");
}

string parts_of_subsystem(DB db,Request const& page,Subsystem_id id){
	auto q=qm<Part_id,Part_state,int>(
		db,
		"SELECT part_id,part_state,qty "
		"FROM part_info "
		"WHERE (id) IN "
			"(SELECT MAX(id) FROM part_info GROUP BY part_id) "
			"AND valid AND subsystem="+as_string(id)
	);
	return h2("Parts in subsystem")+as_table(db,page,vector<Label>{"Part","State","Qty"},q);
}

template<typename T>
optional<T> parse(optional<T> const *x,optional<string> const& a){
	if(!a) return {};
	return parse(x,*a);
}

string subsystems_of_subsystem(DB db,Request const& page,Subsystem_id subsystem){
	return h2("Subsystems in this subsystem")+as_table(
		db,
		page,
		vector<Label>{"Subsystem","Prefix"},
		qm<Subsystem_id,Subsystem_prefix>(
			db,
			"SELECT subsystem_id,prefix "
			"FROM subsystem_info "
			"WHERE "
				"valid AND "
				"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) AND "
				"parent="+escape(subsystem)
		)
	);
}

void inner(ostream& o,Subsystem_editor const& a,DB db){
	auto q=query(
		db,
		"SELECT name,valid,prefix,parent FROM subsystem_info "
		"WHERE subsystem_id="+as_string(a.id)+
		" ORDER BY edit_date DESC LIMIT 1"
	);
	Subsystem_data current;
	if(q.size()==0){
		current.valid=1;
	}else{
		assert(q.size()==1);
		assert(q[0].size()==4);
		current.name=*q[0][0];
		current.valid=stoi(*q[0][1]);
		current.prefix=parse(&current.prefix,*q[0][2]);
		current.parent=parse(&current.parent,q[0][3]);
	}
	make_page(
		o,
		"Subsystem editor",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\"Subsystem_edit\">"
		"<input type=\"hidden\" name=\"subsystem_id\" value=\""+as_string(a.id)+"\">"+
		/*"<br>Name:<input type=\"text\" name=\"name\" value=\""+name+"\">"+
		"<br>Valid:<input type=\"checkbox\" name=\"valid\" "+
			[=](){ if(valid) return "checked=on"; return ""; }()+"\">"+
		show_input(db,"*/
		#define X(A,B) show_input(db,""#B,current.B)+
		SUBSYSTEM_DATA(X)
		#undef X
		"<br><input type=\"submit\">"+
		"</form>"
		+parts_of_subsystem(db,a,a.id)
		+subsystems_of_subsystem(db,a,a.id)
		+h2("History")
		+make_table(
			a,
			{"edit_date","edit_user","id","name","subsystem_id","valid","parent","prefix"},
			query(db,"SELECT edit_date,edit_user,id,name,subsystem_id,valid,parent,prefix FROM subsystem_info WHERE subsystem_id="+as_string(a.id))
		)
	);
}

void inner(ostream& o,Subsystem_edit const& a,DB db){
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
	Subsystem_editor page;
	page.id=Subsystem_id{a.subsystem_id};
	make_page(
		o,
		"Subsystem edit",
		redirect_to(page)
	);
}

void inner(std::ostream& o,Part_new const& a,DB db){
	return inner_new<Part_editor>(o,db,"part");
}

vector<pair<Id,string>> current_subsystems(DB db){
	auto q=query(db,"SELECT subsystem_id,name FROM subsystem_info WHERE (id) IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) AND valid");
	vector<pair<Id,string>> r;
	for(auto row:q){
		r|=make_pair(stoi(*row[0]),*row[1]);
	}
	return r;
}

string subsystem_name(DB db,Id subsystem_id){
	//query(db,"SELECT name FROM subsystem_info");
	//query(db,"SELECT subsystem_part_id,name FROM part_info WHERE (id) IN (SELECT MAX(id) FROM part_info GROUP BY part_id) AND valid"
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

vector<std::string> operator|=(vector<string> &a,const char *s){
	a.push_back(s);
	return a;
}

string done(DB db,Request const& page){
	return h2("Done")+table_with_totals(
		db,
		page,
		vector<Label>{Label{"State"},Label{"Machine"},Label{"Time"},Label{"Subsystem"},Label{"Part"}},
		qm<variant<Part_state,string>,optional<Machine>,Decimal,optional<Subsystem_id>,optional<Part_id>>(
			db,
			"SELECT part_state,machine,time,subsystem,part_id "
			"FROM part_info "
			"WHERE "
				"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
				"AND valid "
				"AND (part_state='fabbed' OR part_state='found' OR part_state='ordered' OR part_state='arrived') "
			"ORDER BY part_state,machine "
		)
	);
}

string to_do(DB db,Request const& page){
	return h2("To do")+table_with_totals(
		db,
		page,
		vector<Label>{Label{"State"},Label{"Machine"},"Time","Subsystem","Part"},
		qm<variant<Part_state,string>,optional<Machine>,Decimal,optional<Subsystem_id>,optional<Part_id>>(
			db,
			"SELECT part_state,machine,time,subsystem,part_id "
			"FROM part_info "
			"WHERE "
				"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
				"AND valid "
				"AND part_state!='fabbed' AND part_state!='found' AND part_state!='ordered' AND part_state!='arrived' "
			"ORDER BY part_state,machine "
		)
	);
}

std::string show_current_parts(DB db,Request const& page){
	return h2("Current state")+to_do(db,page)+done(db,page);
}

void inner(ostream& o,Parts const& a,DB db){
	make_page(
		o,
		"Parts",
		show_current_parts(db,a)+
		show_table(db,a,"part_info","History")
	);
}

void inner(ostream& o,Part_editor const& a,DB db){
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
	make_page(
		o,
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
			a,
			all_cols,
			query(db,"SELECT "+join(",",all_cols)+" FROM "+area_lower+"_info WHERE "+area_lower+"_id="+as_string(a.id))
		)
	);
}

void inner(ostream& o,Meeting_editor const& a,DB db){
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
	make_page(
		o,
		area_cap+" editor",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\""+area_cap+"_edit\">"
		"<input type=\"hidden\" name=\""+area_lower+"_id\" value=\""+as_string(a.id)+"\">"
		#define X(A,B) "<br>"+show_input(db,""#B,current.B)+
		MEETING_DATA(X)
		#undef X
		"<br><input type=\"submit\">"+
		"</form>"
		+h2("History")
		+make_table(
			a,
			all_cols,
			query(db,"SELECT "+join(",",all_cols)+" FROM "+area_lower+"_info WHERE "+area_lower+"_id="+as_string(a.id))
		)
	);
}

void inner(ostream& o,Meeting_new const&,DB db){
	return inner_new<Meeting_editor>(o,db,"meeting");
}

string current_calendar(DB db,Request const& page){
	auto found=qm<Meeting_id,Date,Meeting_length,Color,std::string>(
		db,
		"SELECT meeting_id,date,length,color,notes "
		"FROM meeting_info "
		"WHERE "
			"valid "
			"AND id in (SELECT MAX(id) FROM meeting_info GROUP BY meeting_id)"
			"AND date>now() "
		"ORDER BY date"
	);
//(id) IN "
  //                      "(SELECT MAX(id) FROM subsystem_info WHERE subsystem_id="+as_string(id)+") "

	stringstream ss;
	ss<<h2("Remaining meetings");
	ss<<"<table border>";
	ss<<"<tr>";
	ss<<th("Date")<<th("Length (hours)")<<th("Notes");
	ss<<"</tr>";
	for(auto [meeting_id,date,length,color,notes]:found){
		ss<<"<tr>";
		Meeting_editor page;
		page.id=meeting_id;
		ss<<"<td bgcolor=\""<<color<<"\">"<<link(page,as_string(date))<<"</td>";
		ss<<td(as_string(length));
		ss<<td(notes);
		ss<<"</tr>";
	}
	ss<<"<tr>";
	ss<<td("Total days: "+as_string(found.size()));
	ss<<td("Total hours: "+as_string(sum(mapf([](auto x){ return get<2>(x); },found))));
	ss<<"</tr>";
	ss<<"</table>";
	return ss.str();
	return as_table(db,page,vector<Label>{"Date","Length","Color"},found);
}

void inner(std::ostream& o,Calendar const& a,DB db){
	make_page(
		o,
		"Calendar",
		current_calendar(db,a)
		+to_do(db,a)
		//show_table(db,"meeting")
		+show_table(db,a,"meeting_info","History")
	);
}

void inner(std::ostream& o,Part_edit const& a,DB db){
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
	Part_editor page;
	page.id=Part_id{a.part_id};
	make_page(
		o,
		area_cap+" edit",
		redirect_to(page)
	);
}

void inner(ostream& o,Meeting_edit const& a,DB db){
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
	Meeting_editor page;
	page.id=Meeting_id{a.meeting_id};
	make_page(
		o,
		area_cap+" edit",
		redirect_to(page)
	);
}

void inner(std::ostream& o,Error const& a,DB db){
	make_page(
		o,
		"Error",
		a.s
	);
}

string show_table_user(DB db,Request const& page,Table_name const& name,User const& edit_user){
	auto columns=firsts(query(db,"DESCRIBE "+name));
	stringstream ss;
	ss<<h2(name);
	ss<<"<table border>";
	ss<<"<tr>";
	for(auto elem:columns) ss<<th(as_string(elem));
	ss<<"</tr>";
	for(auto row:query(db,"SELECT * FROM "+name+" WHERE edit_user="+escape(edit_user))){
		ss<<"<tr>";
		for(auto elem:row){
			ss<<td(as_string(elem));
		}
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}


void inner(std::ostream& o,By_user const& a,DB db){
	make_page(
		o,
		"By user \""+as_string(a)+"\"",
		show_table_user(db,a,"subsystem_info",a.user)
		+show_table_user(db,a,"part_info",a.user)
		+show_table_user(db,a,"meeting_info",a.user)
	);
}

template<size_t N,typename ... Ts>
auto get_col(vector<tuple<Ts...>> const& in){
	return mapf([](auto x){ return get<N>(x); },in);
}

string by_machine(DB db,Request const& page,Machine const& a){
	auto qq=qm<Part_state,Subsystem_id,Part_id,unsigned,Decimal>(
		db,
		"SELECT part_state,subsystem,part_id,qty,time "
		"FROM part_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
			"AND valid "
			"AND machine='"+as_string(a)+"' "
		"ORDER BY part_state"
	);
	return h2(as_string(a))
		+as_table(db,page,vector<Label>{"Status","Subsystem","Part","Qty","Time"},qq)
		+"Total time:"+as_string(sum(get_col<4>(qq)));
}

vector<Machine> machines(){
	vector<Machine> r;
	#define X(A) r|=Machine::A;
	MACHINES(X)
	#undef X
	return r;
}

void inner(std::ostream& o,Machines const& a,DB db){
	make_page(
		o,
		"Machines",
		join("",mapf([=](auto x){ return by_machine(db,a,x); },machines()))
	);
}

string to_order(DB db,Request const& page){
	return h2("To order")+as_table(
		db,
		page,
		vector<Label>{"Subsystem","Part","Supplier","Part #","qty","part_link","price"},
		qm<Subsystem_id,Part_id,string,string,unsigned,URL,Decimal>(
			db,
			"SELECT subsystem,part_id,part_supplier,part_number,qty,part_link,price "
			"FROM part_info "
			"WHERE "
				"valid "
				"AND id IN (SELECT max(id) FROM part_info GROUP BY part_id) "
				"AND part_state='buy_list'"
		)
	);
}

string on_order(DB db,Request const& page){
	return h2("On order")+as_table(
		db,
		page,
		vector<Label>{"Subsystem","Part","Supplier","Part #","qty","Expected arrival"},
		qm<Subsystem_id,Part_id,string,string,unsigned,Date>(
			db,
			"SELECT subsystem,part_id,part_supplier,part_number,qty,arrival_date "
			"FROM part_info "
			"WHERE "
				"valid "
				"AND id IN (SELECT max(id) FROM part_info GROUP BY part_id) "
				"AND part_state='ordered' "
			"ORDER BY arrival_date"
		)
	);
}

string arrived(DB db,Request const& page){
	return h2("Arrived")+as_table(
		db,
		page,
		vector<Label>{"Subsystem","Part","Supplier","Part #","qty","Arrival date"},
		qm<Subsystem_id,Part_id,string,string,unsigned,Date>(
			db,
			"SELECT subsystem,part_id,part_supplier,part_number,qty,arrival_date "
			"FROM part_info "
			"WHERE "
				"valid "
				"AND id IN (SELECT max(id) FROM part_info GROUP BY part_id) "
				"AND part_state='arrived'"
			"ORDER BY arrival_date"
		)
	);
}

void inner(ostream& o,Machine_page const& a,DB db){
	make_page(
		o,
		as_string(a.machine),
		by_machine(db,a,a.machine)
	);
}

void inner(ostream& o,State const& a,DB db){
	make_page(
		o,
		as_string(a.state),
		as_table(
			db,
			a,
			vector<Label>{"Subsystem","Part"},
			qm<Subsystem_id,Part_id>(
				db,
				"SELECT subsystem,part_id "
				"FROM part_info "
				"WHERE "
					"valid "
					"AND id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
					"AND part_state='"+as_string(a.state)+"'"
				"ORDER BY subsystem,part_id "
			)
		)
	);
}

void inner(ostream& o,Orders const& a,DB db){
	make_page(
		o,
		"Orders",
		to_order(db,a)
		+on_order(db,a)
		+arrived(db,a)
	);
}

void inner(ostream& o,Extra const&,DB){
	stringstream ss;
	ss<<"extra!";
	system("env");
	make_page(
		o,
		"Extra info",
		ss.str()
	);
}

#define EMPTY_PAGE(X) void inner(ostream& o,X const& x,DB db){ \
	make_page(\
		""#X,\
		as_string(x)+p("Under construction")\
	); \
}
//EMPTY_PAGE(Meeting_editor)
//EMPTY_PAGE(Meeting_edit)
//EMPTY_PAGE(By_user)
#undef EMPTY_PAGE


void run(ostream& o,Request const& req,DB db){
	#define X(A) if(holds_alternative<A>(req)) return inner(o,get<A>(req),db);
	PAGES(X)
	X(Error)
	#undef X
	Error page;
	page.s="Could not find page";
	inner(o,page,db);
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
		run(cout,parse_query(g),db);
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
		PRINT(r);
		stringstream ss;
		run(ss,r,db);
	}
	auto q=parse_query("");
	stringstream ss;
	run(ss,q,db);
	return 0;
}

int main(int argc,char **argv,char **envp){
	try{
		return main1(argc,argv,envp);
	}catch(const char *s){
		cout<<"Caught:"<<s<<"\n";
	}catch(std::string const& s){
		cout<<"Caught:"<<s<<"\n";
	}
}
