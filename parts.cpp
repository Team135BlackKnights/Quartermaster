#include<algorithm>
#include "util.h"
#include "data_types.h"
#include "auth.h"
#include "queries.h"

using namespace std;

//TODO: Move to util
template<typename Func,typename T>
vector<T> filter(Func f,vector<T> v){
	vector<T> r;
	for(auto elem:v){
		if(f(elem)) r|=elem;
	}
	return r;
}

/*Subsystem_id& operator+=(Subsystem_id& a,Subsystem_id b){
	a.data=0;//obviously invalid; real ones start numbering at 1.
	return a;
}*/

template<typename T>
optional<T>& operator+=(optional<T>& a,optional<T> b){
	if(b) a={};
	return a;
}

template<typename T>
variant<T,string>& operator+=(std::variant<T,string>& a,std::variant<T,string> b){
	a="Total";
	return a;
}

Dummy& operator+=(Dummy& a,Dummy){
	return a;
}

template<typename ... Ts>
tuple<Ts...>& operator+=(tuple<Ts...>& a,tuple<Ts...> b){
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
T sum(vector<T> v){
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
vector<T> convert1(vector<vector<optional<string>>> in){
	return mapf(
		[](auto x){
			assert(x.size()==1);
			return parse((T*)0,*x[0]);
		},
		in
	);
}

template<typename ... Ts>
tuple<Ts...> convert_row(vector<optional<string>> row){
	tuple<Ts...> t;
	size_t i=0;
	std::apply([&](auto&&... x){ ((x=parse(&x,*row[i++])), ...); },t);
	return t;
}

template<typename ... Ts>
vector<tuple<Ts...>> convert(vector<vector<optional<string>>> in){
	return mapf(convert_row<Ts...>,in);
}

template<typename T>
vector<T> q1(DB db,string query_string){
	auto q=query(db,query_string);
	//PRINT(q);
	return convert1<T>(q);
}

template<typename ... Ts>
vector<tuple<Ts...>> qm(DB db,string query_string){
	auto q=query(db,query_string);
	return convert<Ts...>(q);
}

template<typename T>
vector<optional<T>> operator|=(vector<optional<T>>,T)nyi

template<typename T>
string pretty_td(DB,T t){
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

string pretty_td(DB,URL a){
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

string pretty_td(DB,User a){
	By_user page;
	page.user=a;
	return td(link(page,as_string(a)));
}

//static const char* ARROW_UP="▲";
//static const char* ARROW_DOWN="▼";

string sortable_labels(Request const& page,vector<string> labels){
	std::stringstream ss;
	ss<<"<tr>";
	for(auto label:labels){
		ss<<"<th>";
		auto p2=page;
		std::visit([&](auto &x){ x.sort_by=label; x.sort_order="asc"; }, p2);
		auto p3=page;
		std::visit([&](auto &x){ x.sort_by=label; x.sort_order="desc"; }, p3);
		//p2.sort_by="dog";
		ss<<label<<" ";
		ss<<link(p2,"/\\");
		ss<<" "<<link(p3,"\\/");
		ss<<"</th>";
	}
	ss<<"</tr>";
	return ss.str();
}

template<std::size_t ...S>
struct seq { };

template<std::size_t N, std::size_t ...S>
struct gens : gens<N-1, N-1, S...> { };

template<std::size_t ...S>
struct gens<0, S...> {
  typedef seq<S...> type;
};

template <template <typename ...> class Tup1,
    template <typename ...> class Tup2,
    typename ...A, typename ...B,
    std::size_t ...S>
auto tuple_zip_helper(Tup1<A...> t1, Tup2<B...> t2, seq<S...> s) ->
decltype(std::make_tuple(std::make_pair(std::get<S>(t1),std::get<S>(t2))...)) {
  return std::make_tuple( std::make_pair( std::get<S>(t1), std::get<S>(t2) )...);
}

template <template <typename ...> class Tup1,
  template <typename ...> class Tup2,
  typename ...A, typename ...B>
auto tuple_zip(Tup1<A...> t1, Tup2<B...> t2) ->
decltype(tuple_zip_helper(t1, t2, typename gens<sizeof...(A)>::type() )) {
  static_assert(sizeof...(A) == sizeof...(B), "The tuple sizes must be the same");
  return tuple_zip_helper( t1, t2, typename gens<sizeof...(A)>::type() );
}

template<typename ... Ts>
vector<tuple<Ts...>> sort_by_col(vector<tuple<Ts...>> a,unsigned element){
	std::sort(
		begin(a),
		end(a),
		[element](auto e1,auto e2)->bool{

			auto t=tuple_zip(e1,e2);
			unsigned at=0;
			bool result=0;
			std::apply(
				[&](auto p){
					if(at==element) result=( p.first<p.second );
					at++;
				},
				t
			);
			return result;
			//return get<N>(a) < get<N>(b);
		}
	);
	return a;
}

template<typename T>
vector<T> sorted(vector<T> a){
	sort(begin(a),end(a));
	return a;
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
string as_table(DB db,Request const& page,vector<string> labels,vector<tuple<Ts...>> const& a){
	stringstream ss;
	ss<<"<table border>";
	ss<<sortable_labels(page,labels);
	
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
			if(label==sort_by){
				return i;
			}
		}
		return {};
	}();

	if(index){
		sort(
			begin(vv),
			end(vv),
			[index,sort_order](auto e1,auto e2){
				if(sort_order=="desc"){
					return e1[*index]>e2[*index];
				}
				return e1[*index]<e2[*index];
			}
		);
	}

	for(auto row:vv){
		ss<<"<tr>";
		for(auto [tag,elem]:row){
			(void)tag;
			ss<<elem;
		}
		ss<<"</tr>";
	}

	/*for(auto row:sorted(a)){
		ss<<"<tr>";
		std::apply([&](auto&&... x){ ((ss<<pretty_td(db,x)),...); },row);
		ss<<"</tr>";
	}*/
	ss<<"</table>";
	return ss.str();
}

template<typename ... Ts>
string table_with_totals(DB db,Request const& page,vector<string> labels,vector<tuple<Ts...>> const& a){
	stringstream ss;
	ss<<"<table border>";
	ss<<join("",mapf(th,labels));
	for(auto row:a){
		ss<<"<tr>";
		std::apply([&](auto&&... x){ ((ss<<pretty_td(db,x)),...); },row);
		ss<<"</tr>";
	}
	if(!a.empty()){
		ss<<"<tr>";
		auto t=sum(a);
		std::apply([&](auto&&... x){ ((ss<<pretty_td(db,x)),...); },t);
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

template<typename T>
string as_table(DB db,Request const& page,vector<string> labels,vector<T> const& a){
	return as_string(
		db,
		page,
		labels,
		mapf(
			[](auto x){ return make_tuple(x); },
			a
		)
	);
}

string as_table(vector<string> labels,vector<vector<std::string>> in){
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

string as_table(vector<string> labels,vector<vector<std::optional<string>>> in){
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

string make_page(string heading,string main_body){
	string name=heading+" - "+title_end();
	return html(
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
	return as_table(db,page,{"State","Subsystem","Part"},a);
}

string inner(Home const& a,DB db){
	return make_page("Home",parts_by_state(db,a));
}

string make_table(Request const& page,vector<string> labels,vector<vector<optional<string>>> const& a){
	stringstream ss;
	ss<<"<table border>";
	ss<<"<tr>";
	for(auto elem:labels) ss<<th(elem);
	ss<<"</tr>";
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

string show_table(DB db,Request const& page,Table_name name,optional<string> title={}){
	auto columns=firsts(query(db,"DESCRIBE "+name));
	stringstream ss;
	ss<<h2(title?*title:name);
	ss<<"<table border>";
	ss<<"<tr>";
	for(auto elem:columns) ss<<th(as_string(elem));
	ss<<"</tr>";
	for(auto row:query(db,"SELECT * FROM "+name)){
		ss<<"<tr>";
		for(auto elem:row){
			ss<<td(as_string(elem));
		}
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

vector<int> get_ids(DB db,Table_name table){
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

State make_state(Part_state a){
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
		{
			"Subsystem"
			#define X(A) ,link(make_state(Part_state::A),""#A)
			PART_STATES(X)
			#undef X
			,"Total"
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
			"Subsystem"
			#define X(A) ,link(make_machine_page(Machine::A),""#A)
			MACHINES(X)
			#undef X
		},
		q
	);
}

string show_current_subsystems(DB db,Request const& page){
	return subsystem_state_count(db,page)+subsystem_machine_count(db,page);
}

string inner(Subsystems const& a,DB db){
	return make_page(
		"Subsystems",
		show_current_subsystems(db,a)/*+
		h2("Debug info")+
		[=](){
			stringstream ss;
			for(auto id:get_ids(db,"subsystem")){
				ss<<link(Subsystem_editor{id},"Edit "+as_string(id));
			}
			return ss.str();
		}()+
		show_table(db,"subsystem")+*/
		+show_table(db,a,"subsystem_info","History")
	);
}

template<typename T>
string redirect_to(T t){
	stringstream ss;
	//ss<<"<meta http-equiv = \"refresh\" content = \"2; url = ";
	ss<<"<meta http-equiv = \"refresh\" content = \"0; url = ";
	ss<<"?"<<to_query(t);
	ss<<"\" />";
	ss<<"You should have been automatically redirected to:"<<t<<"\n";
	return ss.str();
}

template<typename T>
string inner_new(DB db,Table_name table){
	run_cmd(db,"INSERT INTO "+table+" VALUES ()");
	auto q=query(db,"SELECT LAST_INSERT_ID()");
	//PRINT(q);
	assert(q.size()==1);
	assert(q[0].size()==1);
	assert(q[0][0]);
	auto id=stoi(*q[0][0]);

	T page;
	page.id={id};
	return make_page(
		"Subsystem new",
		redirect_to(page)
	);
}

string inner(Subsystem_new const&,DB db){
	return inner_new<Subsystem_editor>(db,"subsystem");
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
	return h2("Parts in subsystem")+as_table(db,page,{"Part","State","Qty"},q);
}

string inner(Subsystem_editor const& a,DB db){
	auto q=query(
		db,
		"SELECT name,valid FROM subsystem_info "
		"WHERE subsystem_id="+as_string(a.id)+
		" ORDER BY edit_date DESC LIMIT 1"
	);
	string name;
	bool valid;
	if(q.size()==0){
		name="";
		valid=1;
	}else{
		assert(q.size()==1);
		assert(q[0].size()==2);
		name=*q[0][0];
		valid=stoi(*q[0][1]);
	}
	return make_page(
		"Subsystem editor",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\"Subsystem_edit\">"
		"<input type=\"hidden\" name=\"subsystem_id\" value=\""+as_string(a.id)+"\">"
		"<br>Name:<input type=\"text\" name=\"name\" value=\""+name+"\">"+
		"<br>Valid:<input type=\"checkbox\" name=\"valid\" "+
			[=](){ if(valid) return "checked=on"; return ""; }()+"\">"+
		"<br><input type=\"submit\">"+
		"</form>"
		+parts_of_subsystem(db,a,a.id)
		+h2("History")
		+make_table(
			a,
			{"edit_date","edit_user","id","name","subsystem_id","valid"},
			query(db,"SELECT edit_date,edit_user,id,name,subsystem_id,valid FROM subsystem_info WHERE subsystem_id="+as_string(a.id))
		)
	);
}

string inner(Subsystem_edit const& a,DB db){
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
	return make_page(
		"Subsystem edit",
		redirect_to(page)
	);
}

string inner(Part_new const& a,DB db){
	return inner_new<Part_editor>(db,"part");
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
		{"State","Machine","Time","Subsystem","Part"},
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
		{"State","Machine","Time","Subsystem","Part"},
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

string inner(Parts const& a,DB db){
	return make_page(
		"Parts",
		show_current_parts(db,a)+
		show_table(db,a,"part_info","History")
	);
}

string inner(Part_editor const& a,DB db){
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
	return make_page(
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

string inner(Meeting_editor const& a,DB db){
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
	return make_page(
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

string inner(Meeting_new const&,DB db){
	return inner_new<Meeting_editor>(db,"meeting");
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
	return as_table(db,page,{"Date","Length","Color"},found);
}

string inner(Calendar const& a,DB db){
	return make_page(
		"Calendar",
		current_calendar(db,a)
		+to_do(db,a)
		//show_table(db,"meeting")
		+show_table(db,a,"meeting_info","History")
	);
}

string inner(Part_edit const& a,DB db){
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
	return make_page(
		area_cap+" edit",
		redirect_to(page)
	);
}

string inner(Meeting_edit const& a,DB db){
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
	return make_page(
		area_cap+" edit",
		redirect_to(page)
	);
}

string inner(Error const& a,DB db){
	return make_page(
		"Error",
		a.s
	);
}

string show_table_user(DB db,Request const& page,Table_name name,User edit_user){
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


string inner(By_user const& a,DB db){
	return make_page(
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
		+as_table(db,page,{"Status","Subsystem","Part","Qty","Time"},qq)
		+"Total time:"+as_string(sum(get_col<4>(qq)));
}

vector<Machine> machines(){
	vector<Machine> r;
	#define X(A) r|=Machine::A;
	MACHINES(X)
	#undef X
	return r;
}

string inner(Machines const& a,DB db){
	return make_page(
		"Machines",
		join("",mapf([=](auto x){ return by_machine(db,a,x); },machines()))
	);
}

string to_order(DB db,Request const& page){
	return h2("To order")+as_table(
		db,
		page,
		{"Subsystem","Part","Supplier","Part #","qty","part_link","price"},
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
		{"Subsystem","Part","Supplier","Part #","qty","Expected arrival"},
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
		{"Subsystem","Part","Supplier","Part #","qty","Arrival date"},
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

string inner(Machine_page const& a,DB db){
	return make_page(
		as_string(a.machine),
		by_machine(db,a,a.machine)
	);
}

string inner(State const& a,DB db){
	return make_page(
		as_string(a.state),
		as_table(
			db,
			a,
			{"Subsystem","Part"},
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

string inner(Orders const& a,DB db){
	return make_page(
		"Orders",
		to_order(db,a)
		+on_order(db,a)
		+arrived(db,a)
	);
}

#define EMPTY_PAGE(X) string inner(X x,DB db){ \
	return make_page(\
		""#X,\
		as_string(x)+p("Under construction")\
	); \
}
//EMPTY_PAGE(Meeting_editor)
//EMPTY_PAGE(Meeting_edit)
//EMPTY_PAGE(By_user)
#undef EMPTY_PAGE


string run(Request req,DB db){
	#define X(A) if(holds_alternative<A>(req)) return inner(get<A>(req),db);
	PAGES(X)
	X(Error)
	#undef X
	Error page;
	page.s="Could not find page";
	return inner(page,db);
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
		cout<<run(parse_query(g),db);
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
		cout<<run(r,db);
	}
	auto q=parse_query("");
	//PRINT(q);
	auto s=run(q,db);
	//PRINT(s);
	return 0;
}

int main(int argc,char **argv,char **envp){
	try{
		return main1(argc,argv,envp);
	}catch(const char *s){
		cout<<"Caught:"<<s<<"\n";
	}
}
