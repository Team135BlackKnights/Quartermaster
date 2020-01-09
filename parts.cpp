#include "parts.h"
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
#include "meeting.h"
#include "order.h"
#include "progress.h"
#include "chart.h"

using namespace std;

template<typename T>
vector<pair<T,T>> adjacent_pairs(vector<T> const& a){
	vector<pair<T,T>> r;
	for(size_t i=1;i<a.size();i++){
		r|=make_pair(a[i-1],a[i]);
	}
	return r;
}

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
			case Part_state::build_list:
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
	return h2("Current state")
		+subsystem_state_count(db,page)
		+subsystem_machine_count(db,page)
		+to_do(db,page)
		+done(db,page);
}

HISTORY_TABLE(part,PART_INFO_ROW)

string history_sum(DB db){
	stringstream o;
	auto q=qm<\
                PART_INFO_ROW(A_COMMA)\
                Dummy\
        >(\
                db,\
                "SELECT "\
                PART_INFO_ROW(B_STR_COMMA)\
                "0 FROM part_info ORDER BY id"\
        );\
        vector<Label> labels;\
        PART_INFO_ROW(LABEL_B)

	//make a table of changes
	o<<h2("Change table");
	for(auto [a,b]:adjacent_pairs(q)){
		o<<"<p>";
		vector<string> as;
		std::apply(
			[&](auto&&... x){
				( ( (as|=as_string(x)) , ... ));
			},
			a
		);

		vector<string> bs;
		std::apply(
			[&](auto&&... x){
				( ( (bs|=as_string(x)) , ... ));
			},
			b
		);

		vector<string> names;
		#define X(A,B) names|=""#B;
		PART_INFO_ROW(X)
		#undef X

		for(auto [name,a1,b1]:zip(names,as,bs)){
			if(a1!=b1){
				o<<name<<": "<<a1<<" -> "<<b1<<"<br>";
			}
		}
		o<<"</p>";
	}
	return o.str();
}

void inner(ostream& o,Parts const& a,DB db){
	make_page(
		o,
		"Parts",
		link(Part_new{},"New part")
		+show_current_parts(db,a)+
		history_part(db,a)//show_table(db,a,"part_info","History")
		//+history_sum(db)
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
	ss<<"<a name=\""<<name<<"\"></a>";
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
	vector<string> tables{"subsystem_info","part_info","meeting_info"};
	string links;
	links+=h2("Contents");
	for(auto table:tables){
		links+="<a href=\"#"+table+"\">"+table+"</a><br>";
	}
	make_page(
		o,
		"By user \""+as_string(a.user)+"\"",
		links
		+show_table_user(db,a,"subsystem_info",a.user)
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
	return "<a name=\""+as_string(a)+"\"></a>"
		+h2(as_string(a))
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
		h2("Contents")+join(
			"<br>",
			mapf(
				[=](auto x){ return "<a href=\"#"+as_string(x)+"\">"+as_string(x)+"</a>"; },
				machines()
			)
		)
		+join("",mapf([=](auto x){ return by_machine(db,a,x); },machines()))
	);
}

/*struct Part_checkbox:Wrap<Part_checkbox,Part_id>{};

string show_input(DB,std::string const& name,Part_checkbox const& a){
	return "<br><input type=\"checkbox\" name=\""+name+":"+as_string(a)+"\">";
}*/

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
					"AND part_state="+escape(a.state)+" "
				"ORDER BY subsystem,part_id "
			)
		)
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

set<User> users(DB db,string table){
	auto q=qm<User>(
		db,
		"SELECT edit_user "
		"FROM "+table+" "
		"GROUP BY edit_user"
	);
	return to_set(mapf(
		[](auto x){ return get<0>(x); },
		q
	));
}

string link(User u){
	By_user page;
	page.user=u;
	return link(page,as_string(u));
}

string user_links(DB db){
	stringstream ss;
	ss<<h2("Active users");
	auto u=users(db,"part_info")|users(db,"meeting_info")|users(db,"subsystem_info");
	for(auto user:u){
		ss<<link(user)<<"<br>\n";
	}
	return ss.str();
}

extern char **environ;

void inner(ostream& o,Extra const&,DB db){
	stringstream ss;

	ss<<export_links();

	ss<<user_links(db);

	ss<<h2("Current environment variables");
	for(unsigned i=1;environ[i];i++){
		ss<<environ[i]<<"<br>\n";
	}

	show_expected_tables(ss);

	ss<<h2("Data consistency")<<check_part_numbers(db);
	ss<<fields_by_state();

	make_page(
		o,
		"Extra info",
		ss.str()
	);
}

void inner(ostream& o,By_supplier const& a,DB db){
	make_page(
		o,
		as_string(a.supplier)+" (Supplier)",
		as_table(
			db,
			a,
			{"State","Subsystem","Part","Part number","qty","Link","Price","Arrival date"},
			qm<Part_state,Subsystem_id,Part_id,Part_number,int,URL,Decimal,Date>(
				db,
				"SELECT part_state,subsystem,part_id,part_number,qty,part_link,price,arrival_date "
				"FROM part_info "
				"WHERE "
				" id IN (SELECT MAX(id) FROM part_info GROUP BY part_id)"
				" AND valid "
				" AND part_supplier="+escape(a.supplier)+" "
				" ORDER BY part_state"
			)
		)
	);
}

#define EMPTY_PAGE(X) void inner(ostream& o,X const& x,DB db){ \
	make_page(\
		o,\
		""#X,\
		as_string(x)+p("Under construction")\
	); \
}
//EMPTY_PAGE(Order_edit)
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

struct Args{
	bool help=0;
	bool tables=0;
	bool alter_tables=0;
	bool plan=0;
	bool progress=0;
};

void help(){
	cout<<"Arguments:\n";
	cout<<"--help\n";
	cout<<"\tShow this message.\n";
	cout<<"--tables\n";
	cout<<"\tShow current tables in db & expected\n";
	cout<<"--alter_tables\n";
	cout<<"\tTry to change the database to match what this program expects.\n";
	cout<<"--plan\n";
	cout<<"\tShow plan for machine scheduling\n";
}

Args parse_args(int argc,char **argv){
	Args r;
	for(int i=1;argv[i];i++){
		string s=argv[i];
		if(s=="--help"){
			r.help=1;
		}else if(s=="--tables"){
			r.tables=1;
		}else if(s=="--alter_tables"){
			r.alter_tables=1;
		}else if(s=="--plan"){
			r.plan=1;
		}else if(s=="--progress"){
			r.progress=1;
		}else{
			cerr<<"Unexpected argument:"<<argv[i]<<"\n";
			help();
			exit(1);
		}
	}
	return r;
}

int tables(DB db){
	cout<<"Expected database tables\n";
	auto current_tables=show_tables(db);
	for(auto [name,contents]:expected_tables()){
		cout<<name<<"\n";
		cout<<"\tName\t"<<"Type\t"<<"Primary key\n";
		for(auto a:contents){
			cout<<"\t"<<a.first<<"\t"<<a.second.first<<"\t"<<a.second.second<<"\n";
		}

		if(current_tables.count(name)){
			auto f=read(db,name);
			if(contents!=f){
				cout<<"Start diff:\n";
				//diff(contents,f);
				//PRINT(f);
				for(auto [e1,e2]:zip_extend(f,contents)){
					if(e1!=e2)
					cout<<"\t"<<e1<<" "<<e2<<"\n";
				}
			}
		}else{
			cout<<"Table does not exist.\n";
		}
	}
	return 0;
}

optional<vector<string>> parse_enum(string s){
	if(s.substr(0,5)!="enum(") return std::nullopt;
	if(s[s.size()-1]!=')') return std::nullopt;

	auto middle=s.substr(5,s.size()-5-1);
	auto sp=split(',',middle);

	auto parse_item=[=](string s){
		assert(s.size());
		assert(s[0]=='\'');
		assert(s[s.size()-1]=='\'');
		return s.substr(1,s.size()-2);
	};

	return mapf(parse_item,sp);
}

pair<int,int> parse_decimal(string s){
	if(s=="decimal(8,2)") return make_pair(8,2);
	if(s=="decimal(8,3)") return make_pair(8,3);
	nyi
}

void alter_column(DB db,Table_name table,pair<string,Column_type> c1,pair<string,Column_type> c2){
	auto name1=c1.first;
	auto name2=c2.first;
	assert(name1==name2);

	auto t1=c1.second;
	auto t2=c2.second;
	assert(!t1.second);
	assert(!t2.second);

	PRINT(table);
	PRINT(t1);
	PRINT(t2);

	auto e1=parse_enum(t1.first);
	auto e2=parse_enum(t2.first);

	if(e1 && e2){
		auto missing=to_set(*e1)-to_set(*e2);
		if(missing.size()){
			cout<<"Does not cover all previous cases";
			nyi
		}

		run_cmd(db,"ALTER TABLE "+table+" MODIFY "+name1+" "+t2.first);
		return;
	}

	auto d1=parse_decimal(t1.first);
	auto d2=parse_decimal(t2.first);
	if(d2.first>=d1.first && d2.second>=d1.second){
		run_cmd(db,"ALTER TABLE "+table+" MODIFY "+name1+" "+t2.first);
		return;
	}

	nyi
}

vector<string> alter_table(DB db,Table_name table_name,vector<pair<string,Column_type>> after){
	vector<string> queries;
	auto current=read(db,table_name);
	for(auto [e1,e2]:zip_extend(current,after)){
		if(e1!=e2){
			if(!e1){
				//add columns at the end
				stringstream ss;
				ss<<"ALTER TABLE "+table_name+" ADD "+e2->first+" "+e2->second.first;
				if(e2->second.second){
					ss<<" UNIQUE PRIMARY KEY";
				}
				queries|=ss.str();
			}else{
				if(e2){
					//change already-existing columns
					if(e1->first==e2->first){
						alter_column(db,table_name,*e1,*e2);
					}else{
						cout<<"Change w/ possibly totally unrelated column.  Not automatically updating.\n";
						PRINT(table_name);
						PRINT(e1);
						PRINT(e2);
						exit(1);
					}
				}else{
					//delete column
					nyi
				}
			}
		}
	}
	return queries;
}

int alter_tables(DB db){
	//First, create a list of all the edits to make; don't start if don't know what want to do for each of the changes
	vector<string> queries;
	auto existing_tables=show_tables(db);
	//not going to worry about missing tables here to start with.
	for(auto [table_name,after]:expected_tables()){
		if(existing_tables.count(table_name)){
			queries|=alter_table(db,table_name,after);
		}else{
			//queries|=create_table(db,table_name,after);
		}
	}

	print_lines(queries);

	//Now apply all the changes...
	for(auto q:queries) run_cmd(db,q);
	return 0;
}

int main1(int argc,char **argv,char **envp){
	auto args=parse_args(argc,argv);
	if(args.help){
		help();
		return 0;
	}

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

	if(args.tables){
		return tables(db);
	}
	if(args.alter_tables){
		return alter_tables(db);
	}
	if(args.plan){
		make_plan(db);
		return 0;
	}
	if(args.progress){
		progress(db);
		return 0;
	}

	/*auto g1=getenv("HTTP_REFERER");
	auto p=parse_referer(g1);
	cout<<"ref from:"<<p<<"\n";*/

	check_database(db);

	auto g=getenv("QUERY_STRING");
	if(g){
		run(cout,parse_query(g),db);
		return 0;
	}
	auto s=check_part_numbers(db);
	//PRINT(s);
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
