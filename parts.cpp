#include<algorithm>
#include<string.h>
#include<functional>
#include "util.h"
#include "data_types.h"
#include "auth.h"
#include "queries.h"
#include "export.h"
#include "home.h"
#include "subsystems.h"
#include "subsystem.h"
#include "part.h"

using namespace std;

template<typename T>
vector<T> operator|(vector<T> a,T b){
	a|=b;
	return a;
}

template<typename T>
set<T> operator-(set<T> a,T t){
	a.erase(t);
	return a;
}

template<typename T>
vector<T>& operator|=(vector<T> &a,optional<T> t){
	if(t) a|=*t;
	return a;
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

vector<int> get_ids(DB db,Table_name const& table){
	vector<int> r;
	for(auto row:query(db,"SELECT id FROM "+table)){
		r|=stoi(*row.at(0));
	}
	return r;
}

template<typename T>
void inner_new(ostream& o,DB db,Table_name const& table){
	auto id=new_item(db,table);
	T page;
	page.id={id};
	make_page(
		o,
		"New item",
		redirect_to(page)
	);
}

vector<pair<Subsystem_id,string>> current_subsystems(DB db){
	auto q=qm<Subsystem_id,string>(db,"SELECT subsystem_id,name FROM subsystem_info WHERE (id) IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) AND valid");
	vector<pair<Subsystem_id,string>> r;
	for(auto row:q){
		r|=make_pair(get<0>(row),get<1>(row));
	}
	return r;
}

string done(DB db,Request const& page){
	return h2("Done")+table_with_totals(
		db,
		page,
		vector<Label>{Label{"State"},Label{"Machine",Machines{}},Label{"Time",Calendar{}},Label{"Subsystem",Subsystems{}},Label{"Part"}},
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
		vector<Label>{Label{"State"},Label{"Machine",Machines{}},Label{"Time",Calendar{}},Label{"Subsystem",Subsystems{}},"Part"},
		qm<variant<Part_state,string>,optional<Machine>,optional<Decimal>,optional<Subsystem_id>,optional<Part_id>>(
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

string check_part_numbers(DB db){
	auto asm_info=qm<Subsystem_id,optional<string>,optional<Subsystem_id>,Subsystem_prefix>(
		db,
		"SELECT subsystem_id,name,parent,prefix "
		"FROM subsystem_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) "
			"AND valid"
	);
	//for every assembly, its parent should either be NULL or something that will eventually get to NULL.
	//if this is not true, don't bother caring about the part prefixes

	map<Subsystem_id,optional<Subsystem_id>> parents;
	for(auto q:asm_info){
		parents[get<0>(q)]=get<2>(q);
	}

	std::function<optional<vector<Subsystem_id>>(Subsystem_id,vector<Subsystem_id>)> find_loop;

	//auto find_loop=[&](Subsystem_id at,vector<Subsystem_id> visited)->optional<vector<Subsystem_id>>{
	find_loop=[&](Subsystem_id at,vector<Subsystem_id> visited)->optional<vector<Subsystem_id>>{
		auto n=parents[at];
		if(!n) return {};
		auto next_visited=visited|*n;
		if(to_set(visited).count(*n)){
			return next_visited;
		}
		return find_loop(*n,next_visited);
	};

	vector<vector<Subsystem_id>> loops;
	for(auto [subsystem,parent]:parents){
		(void)parent;
		auto f=find_loop(subsystem,{});
		loops|=f;
	}
	if(loops.size()){
		stringstream ss;
		ss<<"Found loops:<br>\n";
		for(auto l:loops){
			ss<<l<<"<br>";
			for(auto id:l){
				ss<<subsystem_name(db,id)<<" ";
			}
			ss<<"<br>\n";
		}
		return ss.str();
	}

	map<Subsystem_prefix,set<Subsystem_id>> id_by_prefix;
	map<Subsystem_id,Subsystem_prefix> prefix_by_id;
	for(auto [id,name,parent,prefix]:asm_info){
		(void)name;
		(void)parent;
		id_by_prefix[prefix]|=id;
		prefix_by_id[id]=prefix;
	}

	auto path=[&](Subsystem_id a)->vector<Subsystem_id>{
		//the path between a subsystem id and the root.
		vector<Subsystem_id> r;
		optional<Subsystem_id> at=a;
		while(at){
			at=parents[*at];
			if(at) r|=*at;
		}
		return r;
	};

	auto child=[&](Subsystem_id a,Subsystem_id b){
		//see if b is a child of a.
		return to_set(path(b)).count(a);
	};

	stringstream errors;
	vector<string> bad_prefix;
	//for an assembly, its prefix should either be the same as its parent, or not exist in anything besides its children
	for(auto a:asm_info){
		auto id=get<0>(a);
		auto parent=get<2>(a);
		auto prefix=get<3>(a);
		if(parent && prefix_by_id[*parent]==prefix){
			continue;
		}
		for(auto other:id_by_prefix[prefix]-id){
			if(!child(id,other)){
				stringstream ss;
				ss<<"Shared prefix:"<<a<<" "<<other<<"\n";
				bad_prefix|=ss.str();
			}
		}
	}
	if(bad_prefix.size()){
		errors<<"Bad assembly prefixes:<br>";
		for(auto elem:bad_prefix){
			errors<<elem<<"<br>";
		}
	}

	//for every part in the design, see if the part number looks ok & is unique, consistent with its assembly, etc.
	auto part_info=qm<Part_id,string,Part_number,Subsystem_id,Part_state>(
		db,
		"SELECT part_id,name,part_number,subsystem,part_state "
		"FROM part_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
			"AND valid "
	);

	//part number the same -> name the same
	map<Part_number,set<string>> name_by_number;
	for(auto [id,name,pn,sub,state]:part_info){
		(void)id;
		(void)sub;
		(void)state;
		name_by_number[pn]|=name;
	}

	for(auto [pn,names]:name_by_number){
		if(names.size()>1){
			errors<<"Multiple names for part #("<<pn<<"):"<<names<<"<br>\n";
		}
	}

	//prefix is consistent w/ some subsystem
	for(auto [id,name,pn,sub,state]:part_info){
		(void)id;
		(void)name;
		(void)sub;
		(void)state;
		optional<Part_number_local> pnl;
		try{
			pnl=Part_number_local(pn);
		}catch(...){
		}
		if(pnl){
			cout<<"PN:"<<pnl<<"\n";
		}
	}

	//looks like one of our part numbers if something that we're supposed to build.
	for(auto [id,name,pn,sub,state]:part_info){
		(void)id;
		(void)sub;
		switch(state){
			case Part_state::need_prints:
			case Part_state::need_to_cam:
			case Part_state::cut_list:
			case Part_state::_3d_print:
			case Part_state::fab:
			case Part_state::fabbed:{
				//must have own part number
				optional<Part_number_local> pnl;
				try{
					pnl=Part_number_local(pn);
				}catch(string const& s){
					errors<<"Expected local part number but not found for "<<name<<" ("<<pn<<"):\""<<s<<"\"<br>\n";
				}catch(const char *s){
					errors<<"Expected local part number but not found for "<<name<<" ("<<pn<<"):\""<<s<<"\"<br>\n";
				}
				break;
			}
			case Part_state::in_design:
			case Part_state::find:
			case Part_state::found:
			case Part_state::buy_list:
			case Part_state::ordered:
			case Part_state::arrived:
				//not required to have own part number
				//so don't check anything
				break;
			default:
				assert(0);
		}
	}

	return errors.str();
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
	if(q.size()==0){
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
}

void inner(std::ostream& o,Calendar const& a,DB db){
	make_page(
		o,
		"Calendar",
		current_calendar(db,a)
		+to_do(db,a)
		+show_table(db,a,"meeting_info","History")
	);
}

void inner(ostream& o,Meeting_edit const& a,DB db){
	string area_lower="meeting";
	string area_cap="Meeting";

	vector<pair<string,string>> v;
	v|=pair<string,string>("edit_date","now()");
	v|=pair<string,string>("edit_user",escape(current_user()));
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

void show_expected_tables(std::ostream& o){
	o<<h2("Expected database tables");
	for(auto [name,contents]:expected_tables()){
		o<<h3(name);
		o<<"<table border>";
		o<<tr(th("Name")+th("Type")+th("Primary key"));
		for(auto a:contents){
			o<<tr(td(a.first)+td(a.second.first)+td(as_string(a.second.second)));
		}
		o<<"</table>";
	}
}

extern char **environ;

void inner(ostream& o,Extra const&,DB db){
	stringstream ss;

	ss<<export_links();

	ss<<h2("Current environment variables");
	for(unsigned i=1;environ[i];i++){
		ss<<environ[i]<<"<br>\n";
	}

	show_expected_tables(ss);

	ss<<h2("Data consistency")<<check_part_numbers(db);

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
	auto s=check_part_numbers(db);
	PRINT(s);
	//nyi
	for(auto _:range(100)){
		(void)_;
		auto r=rand((Request*)nullptr);
		PRINT(r);
		PRINT(to_query(r));
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

std::ostream& operator<<(std::ostream& o,std::invalid_argument const& a){
	o<<"invalid_argument(";
	o<<a.what();
	return o<<")";
}

int main(int argc,char **argv,char **envp){
	try{
		return main1(argc,argv,envp);
	}catch(const char *s){
		cout<<"Caught:"<<s<<"\n";
	}catch(std::string const& s){
		cout<<"Caught:"<<s<<"\n";
	}catch(std::invalid_argument const& a){
		cout<<"Caught:"<<a<<"\n";
	}
}
