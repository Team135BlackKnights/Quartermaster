#include<algorithm>
#include<string.h>
#include "util.h"
#include "data_types.h"
#include "auth.h"
#include "queries.h"
#include "export.h"
#include "home.h"
#include "subsystems.h"
#include "subsystem.h"

using namespace std;

template<typename T>
T parse(T const* t,const char *s){
	if(!s) return {};
	return parse(t,string{s});
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

void inner(std::ostream& o,Part_new const& a,DB db){
	//inner_new<Subsystem_editor>(o,db,"subsystem");
	auto id=new_item(db,"part");

	if(a.subsystem){
		auto q="INSERT INTO part_info ("
			"part_id,"
			"edit_user,"
			"edit_date,"
			"valid,"
			"subsystem"
			") VALUES ("
			+escape(id)+","
			+escape(current_user())+","
			"now(),"
			"1,"
			+escape(a.subsystem)
			+")";
		run_cmd(db,q);
	}
	Part_editor page;
	page.id={id};
	make_page(
		o,
		"New part",
		redirect_to(page)
	);
	//return inner_new<Part_editor>(o,db,"part");
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

bool should_show(Part_state state,string name){
	if(name=="valid") return 1;
	if(name=="subsystem") return 1;
	if(name=="name") return 1;
	if(name=="part_number") return 1;
	if(name=="part_state") return 1;
	if(name=="length" || name=="width" || name=="thickness" || name=="material"){
		return state!=Part_state::in_design;
	}
	if(name=="qty") return 1;
	if(name=="time"){
		return state!=Part_state::buy_list && state!=Part_state::ordered && state!=Part_state::arrived;
	}
	if(name=="manufacture_date" || name=="who_manufacture"){
		return state!=Part_state::in_design &&
			state!=Part_state::need_prints &&
			state!=Part_state::buy_list &&
			state!=Part_state::ordered &&
			state!=Part_state::arrived;
	}
	if(name=="machine"){
		return state!=Part_state::find &&
			state!=Part_state::found &&
			state!=Part_state::buy_list &&
			state!=Part_state::ordered &&
			state!=Part_state::arrived;
	}
	if(name=="place"){
		return state==Part_state::find ||
			state==Part_state::found ||
			state==Part_state::fabbed ||
			state==Part_state::arrived;
	}
	if(name=="bent" || name=="bend_type"){
		return state==Part_state::in_design ||
			state==Part_state::need_prints ||
			state==Part_state::need_to_cam ||
			state==Part_state::cut_list ||
			state==Part_state::fabbed;
	}
	if(name=="drawing_link") return 1;
	if(name=="cam_link"){
		return state==Part_state::need_to_cam ||
			state==Part_state::cut_list ||
			state==Part_state::fab ||
			state==Part_state::fabbed ||
			state==Part_state::_3d_print;
	}
	if(name=="part_supplier" || name=="part_link" || name=="arrival_date" || name=="price"){
		return state==Part_state::buy_list ||
			state==Part_state::ordered ||
			state==Part_state::arrived;
	}
	return 1;
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
	if(q.size()==0){
		current.valid=1;
	}else{
		assert(q.size()==1);
		assert(q[0].size()==data_cols.size());
		int i=0;
		#define X(A,B) {\
			auto x=parse((optional<A>*)nullptr,q[0][i]); \
			if(x) current.B=*x;\
			i++;\
		}
		PART_DATA(X)
		#undef X
	}
	vector<string> all_cols=vector<string>{"edit_date","edit_user","id",area_lower+"_id"}+data_cols;
	make_page(
		o,
		area_cap+" editor",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\""+area_cap+"_edit\">"
		"<input type=\"hidden\" name=\""+area_lower+"_id\" value=\""+as_string(a.id)+"\">"+
		#define X(A,B) [&]()->string{ if(should_show(current.part_state,""#B)) return show_input(db,""#B,current.B); else return ""; }()+
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

void inner(std::ostream& o,Part_edit const& a,DB db){
	string area_lower="part";
	string area_cap="Part";

	vector<pair<string,string>> v;
	v|=pair<string,string>("edit_date","now()");
	v|=pair<string,string>("edit_user",escape(current_user()));
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

extern char **environ;

void inner(ostream& o,Extra const&,DB){
	stringstream ss;
	ss<<export_links();
	ss<<h2("Current environment variables");
	for(unsigned i=1;environ[i];i++){
		ss<<environ[i]<<"<br>\n";
	}
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
