#include "meeting.h"
#include<functional>
#include "subsystems.h"
#include "subsystem.h"
#include "part.h"

using namespace std;

void print_r(bool)nyi

void print_r(size_t i,Date d){
	indent(i);
	cout<<d<<"\n";
}

template<typename K,typename V>
void print_r(size_t i,map<K,V> const& a){
	indent(i);
	cout<<"map\n";
	for(auto p:a){
		print_r(i+1,p);
	}
}

template<typename T>
void print_r(size_t i,T t){
	indent(i);
	cout<<t<<"\n";
}

template<typename A,typename B>
void print_r(size_t i,pair<A,B> const& a){
	indent(i);
	cout<<"pair\n";
	print_r(i+1,a.first);
	print_r(i+1,a.second);
}

template<typename T>
void print_r(size_t i,vector<T> const& a){
	indent(i);
	cout<<"vec\n";
	for(auto elem:a){
		print_r(i+1,elem);
	}
}

template<typename T>
void print_r(T t){
	print_r(0,t);
}

string link(Meeting_id a,string body){
	Meeting_editor r;
	r.id=a;
	return link(r,body);
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

std::string show_plan(DB db,Request const& page);

std::string input_table(vector<Input> const& a){
	stringstream ss;
	ss<<"<table>";
	for(auto elem:a){
		auto s=as_string(elem.name);
		if(s[0]!='<'){
			ss<<"<tr id=\""<<elem.name<<"\">";
		}else{
			ss<<"<tr id=\"subsystem\">";
		}
		ss<<"<td align=right>"<<elem.name<<"</td>";
		ss<<td(elem.form)<<td(elem.notes);
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
}

void inner(ostream& o,Meeting_editor const& a,DB db){
	string user = current_user();
	if (user == "no_user") {
		cout << "Location: /cgi-bin/login.cgi\n\n";
	}
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
		"WHERE "+area_lower+"_id="+escape(a.id)+
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
		as_string(current.date)+" (meeting)",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\""+area_cap+"_edit\">"
		"<input type=\"hidden\" name=\""+area_lower+"_id\" value=\""+escape(a.id)+"\">"
		+after_done()
		+input_table([=](){
			vector<Input> r;
			#define X(A,B) r|=show_input(db,""#B,current.B);
			MEETING_DATA(X)
			#undef X
			return r;
		}())+
		"<br><input type=\"submit\">"+
		"</form>"
		+h2("History")
		+make_table(
			a,
			all_cols,
			query(db,"SELECT "+join(",",all_cols)+" FROM "+area_lower+"_info WHERE "+area_lower+"_id="+escape(a.id))
		)
	);
}

void inner(ostream& o,Meeting_new const&,DB db){
	string user = current_user();
	if (user == "no_user") {
		cout << "Location: /cgi-bin/login.cgi\n\n";
	}
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

HISTORY_TABLE(meeting,MEETING_INFO_ROW)

void inner(std::ostream& o,Calendar const& a,DB db){
	string user = current_user();
	if (user == "no_user") {
		cout << "Location: /cgi-bin/login.cgi\n\n";
	}
	make_page(
		o,
		"Calendar",
		link(Meeting_new{},"New meeting")
		+"<br>"+link(State_change{},"State changes by date")
		+"<br>"+link(Chart{},"Chart")
		+current_calendar(db,a)
		+to_do(db,a)
		+show_plan(db,a)
		+history_meeting(db,a)//+show_table(db,a,"meeting_info","History")
	);
}

using Item=variant<Part_id,Subsystem_id>;
using Time=Decimal;
using Schedule_item=pair<Time,Item>;
using To_do=map<Machine,vector<Schedule_item>>;

#define MEETING_PLAN_ITEMS(X)\
	X(Decimal,length)\
	X(To_do,to_do)

struct Meeting_plan{
	//at some point, could decide that there are different amounts of time available on each of these.
	MEETING_PLAN_ITEMS(INST)
};

std::ostream& operator<<(std::ostream& o,Meeting_plan const& a){
	o<<"Meeting_plan( ";
	#define X(A,B) o<<""#B<<":"<<a.B<<" ";
	MEETING_PLAN_ITEMS(X)
	#undef X
	return o<<")";
}

void print_r(size_t i,Meeting_plan const& a){
	indent(i);
	cout<<"Meeting_plan\n";
	#define X(A,B) print_r(i+1,a.B);
	MEETING_PLAN_ITEMS(X)
	#undef X
}

using Plan=vector<pair<Date,Meeting_plan>>;

Plan blank_plan(DB db){
	auto q=qm<Date,Decimal>(
		db,
		"SELECT date,length "
		"FROM meeting_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM meeting_info GROUP BY meeting_id) "
			"AND valid "
			"AND date>now() "
		"ORDER BY date"
	);
	return mapf(
		[](auto x){
			return make_pair(
				get<0>(x),
				Meeting_plan{get<1>(x),{}}
			);
		},
		q
	);
}

#define BUILD_ITEMS(X)\
	X(Item,item)\
	X(Machine,machine)\
	X(Decimal,length)\
	X(set<Item>,dependencies)\
	X(std::optional<Date>,wait)\
	X(Priority,priority)\

struct Build_item{
	BUILD_ITEMS(INST)
};

bool operator<(Build_item const& a,Build_item const& b){
	#define X(A,B) if(a.B<b.B) return 1; if(b.B<a.B) return 0;
	BUILD_ITEMS(X)
	#undef X
	return 0;
}

bool operator==(Build_item const& a,Build_item const& b){
	#define X(A,B) if(a.B!=b.B) return 0;
	BUILD_ITEMS(X)
	#undef X
	return 1;
}

bool operator!=(Build_item const& a,Build_item const& b){
	return !(a==b);
}

std::ostream& operator<<(std::ostream& o,Build_item const& a){
	o<<"Build_item( ";
	#define X(A,B) o<<""#B<<":"<<a.B<<" ";
	BUILD_ITEMS(X)
	#undef X
	return o<<")";
}

#define X(NAME) void print_r(size_t i,NAME const&){\
	indent(i);\
	cout<<""#NAME<<"\n";\
}
X(Build_item)
#undef X

std::string table(DB db,Request const& page,vector<Build_item> const& a){
	vector<Label> labels;
	#define X(A,B) labels|=Label{""#B};
	BUILD_ITEMS(X)
	#undef X
	
	using T=tuple<
		#define X(A,B) A,
		BUILD_ITEMS(X)
		#undef X
		Dummy
	>;
	vector<T> v;
	for(auto elem:a){
		T t{
			#define X(A,B) elem.B,
			BUILD_ITEMS(X)
			#undef X
			{}
		};
		v|=t;
	}
	return as_table<
		#define X(A,B)
		BUILD_ITEMS(X)
		#undef X
	>(db,page,labels,v);
}

std::string table(DB db,Request const& page,vector<pair<Build_item,string>> const& a){
	vector<Label> labels;
	#define X(A,B) labels|=Label{""#B};
	BUILD_ITEMS(X)
	#undef X
	labels|=Label{"Reason"};

	using T=tuple<
		#define X(A,B) A,
		BUILD_ITEMS(X)
		#undef X
		string
	>;
	vector<T> v;
	for(auto elem:a){
		T t{
			#define X(A,B) elem.first.B,
			BUILD_ITEMS(X)
			#undef X
			elem.second
		};
		v|=t;
	}
	return as_table<
		#define X(A,B)
		BUILD_ITEMS(X)
		#undef X
	>(db,page,labels,v);
}

vector<Build_item> to_build(DB db){
	auto q=qm<Part_id,Machine,Decimal,Subsystem_id>(
		db,
		"SELECT part_id,machine,GREATEST(time,0.1),subsystem "
		"FROM part_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
			"AND valid "
			"AND (part_state='cut_list' OR part_state='find' OR part_state='_3d_print' OR part_state='fab') "
		"ORDER BY part_id"
	);

	vector<Build_item> r;
	for(auto row:q){
		r|=Build_item{
			get<0>(row),
			get<1>(row),
			get<2>(row),
			{},
			std::nullopt,
			0
		};
	}

	map<Subsystem_id,set<Item>> dependencies;
	for(auto [part,_1,_2,subsystem]:q){
		(void)_1;
		(void)_2;
		dependencies[subsystem].insert(part);
	}

	const auto wait_items=to_map(qm<Subsystem_id,Date>(
		db,
		"SELECT subsystem,MAX(arrival_date) "
		"FROM part_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
			"AND valid "
			"AND (part_state='on_order')"
		"GROUP BY subsystem "
		"ORDER BY subsystem "
	));

	auto q1=qm<Subsystem_id,Decimal,optional<Subsystem_id>,Priority>(
		db,
		"SELECT subsystem_id,GREATEST(time,.1),parent,priority "
		"FROM subsystem_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) "
			"AND valid "
			"AND (state='parts' OR state='assembly') "
		"ORDER BY subsystem_id"
	);

	for(auto [id,_1,parent,_3]:q1){
		(void)_1;
		(void)_3;
		if(parent){
			dependencies[*parent].insert(id);
		}
	}

	for(auto row:q1){
		auto id=get<0>(row);
		r|=Build_item{
			id,
			Machine::none,
			get<1>(row),
			dependencies[id],
			[=]()->std::optional<Date>{
				auto f=wait_items.find(id);
				if(f==wait_items.end()){
					return std::nullopt;
				}
				return f->second;
			}(),
			get<3>(row)
		};
	}

	//print_lines(sorted(r));
	auto find_elem=[&](Item a)->Build_item*{
		for(auto& x:r){
			if(x.item==a) return &x;
		}
		return NULL;
		/*print_lines(r);
		PRINT(a);
		assert(0);*/
	};

	std::function<void(Item,Priority)> inherit_priority;
	inherit_priority=[&](Item a,Priority priority){
		//cout<<"inherit:"<<a<<" "<<priority<<"\n";
		auto *e=find_elem(a);
		if(!e) return;
		auto& elem=*e;
		elem.priority=max(elem.priority,priority);
		if(holds_alternative<Subsystem_id>(a)){
			for(auto item:dependencies[get<Subsystem_id>(a)]){
				inherit_priority(item,elem.priority);
			}
		}
		//cout<<"done\n";
	};

	for(auto [parent,_]:dependencies){
		(void)_;
		inherit_priority(Item{parent},Priority{0});
	}

	return r;
}

Decimal machine_time_left(Meeting_plan mp,Machine machine){
	auto total_time_used=sum(
		firsts(flatten(values(mp.to_do)))
	);
	
	auto overall_time_available=mp.length*3-total_time_used;
	
	auto machine_time_used=sum(firsts(mp.to_do[machine]));
	auto machine_time_available=mp.length-machine_time_used;

	return min(overall_time_available,machine_time_available);
}

vector<Build_item> sort_by_priority(vector<Build_item> v){
	sort(
		begin(v),
		end(v),
		[](auto a,auto b){
			return a.priority>b.priority;
		}
	);
	return v;
}

vector<Build_item> topological_sort(vector<Build_item> a){
	vector<Build_item> r;
	set<Item> done;

	auto run_pass=[&](auto in){
		set<Build_item> deferred;
		for(auto elem:sort_by_priority(to_vec(in))){
			if( (elem.dependencies-done).size() ){
				deferred|=elem;
			}else{
				r|=elem;
				done|=elem.item;
			}
		}
		return deferred;
	};

	auto d=run_pass(a);
	while(d.size()){
		auto d2=run_pass(d);
		assert(d2!=d);//if this happens, we have a circular dependency.
		d=d2;
	}
	return r;
}

using Schedule_result=pair<Plan,vector<pair<Build_item,string>>>;

Schedule_result schedule(Plan plan,vector<Build_item> to_schedule){
	map<Item,Date> finish;

	auto schedule_part=[&](auto &x)->std::optional<std::string>{
		vector<optional<Date>> dates1;
		dates1|=mapf(
			[=](auto item)->optional<Date>{
				auto f=finish.find(item);
				//assert(f!=finish.end()); //otherwise, dependency not met.
				if(f==finish.end()) return std::nullopt;
				return f->second;
			},
			x.dependencies
		);
		if(!all(dates1)){
			return "Dependency not met";
		}
		dates1|=x.wait;
		auto dates=non_null(dates1);
		auto dep_date=max(dates);

		for(auto &meeting:plan){
			//skip if before deps are done.
			if(dep_date && meeting.first<*dep_date){
				continue;
			}

			auto m=machine_time_left(meeting.second,x.machine);
			if(m>0){
				auto to_use=min(m,x.length);
				x.length-=to_use;
				meeting.second.to_do[x.machine]|=make_pair(to_use,x.item);
				if(x.length==0){
					finish[x.item]=meeting.first;
					return std::nullopt;
				}
			}
		}
		/*stringstream ss;
		ss<<"Planned so far:\n";
		//print_lines(plan);
		for(auto elem:plan){
			ss<<elem<<"<br>\n";
		}
		ss<<"Could not schedule item.  Out of meetings.\n";
		ss<<"Attempting to schedule:"<<x<<"\n";
		return ss.str();*/
		return "Out of meetings";
	};

	vector<pair<Build_item,string>> failures;
	for(auto x:topological_sort(to_schedule)){
		while(x.length>0){
			auto s=schedule_part(x);
			if(s){
				failures|=make_pair(x,*s);
				x.length=0;
			}
		}
	}
	return make_pair(plan,failures);
}

pair<Plan,vector<pair<Build_item,string>>> make_plan_inner(DB db){
	//this is going to be a graph problem...
	//if you cared that much about it

	//first, going to just schedule building the parts and not care about assembly.
	auto plan=blank_plan(db);
	//print_lines(plan);
	auto to_schedule=to_build(db);
	//print_lines(to_schedule);
	return schedule(plan,to_schedule);
}

std::string show_plan(DB db,Request const& page){
	auto x=make_plan_inner(db);
	stringstream o;
	o<<h2("Scheduled tasks");
	o<<"Methodology notes:\n";
	o<<"<ul>";
	o<<li("Items that are in still in design are totally ignored.");
	o<<li("Items that are yet to be ordered are totally ignored");
	o<<li("Items are given a minimum time estimate of 0.1 hours.");
	o<<li("Assumes that the amount of time for any given machines is equal to meeting length and overall time is 3x meeting length");
	o<<"</ul>";
	o<<"<table border>";
	o<<tr(join("",mapf(th,vector{"Date","Length","Notes","To do"})));
	auto q=qm<Date,Meeting_id,string,string>(
		db,
		"SELECT date,meeting_id,notes,color FROM meeting_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM meeting_info GROUP BY meeting_id) "
			"AND valid"
	);
	auto by_date=to_map(q);
	for(auto a:x.first){
		auto [date,meeting_id,notes,color]=by_date[a.first];
		(void)date;
		o<<"<tr>";
		o<<"<td bgcolor=\""<<color<<"\">"<<link(meeting_id,as_string(a.first))<<"</td>";
		auto mp=a.second;
		o<<td(as_string(mp.length));
		o<<td(notes);
		o<<"<td>";
		o<<"<table border>";
		o<<tr(join(mapf(th,vector{"Machine","Parts"})));
		for(auto [machine,items]:mp.to_do){
			o<<"<tr>";
			o<<td(as_string(machine));
			o<<"<td>";
			o<<"<table border>";
			o<<tr(join(mapf(th,vector{"Time","Part"})));
			for(auto [time,item]:items){
				o<<"<tr>";
				o<<td(as_string(time));
				o<<pretty_td(db,item);
				o<<"</tr>";
			}
			o<<"</table>";
			o<<"</td>";
			o<<"</tr>";
		}
		o<<"</table>";
		o<<"</td>";
		o<<"</tr>";
	}
	o<<"</table>";

	if(x.second.size()){
		o<<h3("Could not be scheduled");
		o<<table(db,page,x.second);
	}
	return o.str();
}

void make_plan(DB db){
	auto x=make_plan_inner(db);
	print_lines(x.first);
	cout<<x.second<<"\n";
}

void inner(ostream& o,Meeting_edit const& a,DB db){
	string user = current_user();
	if (user == "no_user") {
		cout << "Location: /cgi-bin/login.cgi\n\n";
	}
	string area_lower="meeting";
	string area_cap="Meeting";

	vector<pair<string,string>> v;
	v|=pair<string,string>("edit_date","now()");
	v|=pair<string,string>("edit_user",escape(current_user()));
	#define X(A,B) v|=pair<string,string>(""#B,escape(a.B));
	MEETING_EDIT_DATA_ITEMS(X)
	#undef X
	insert(db,area_lower+"_info",v);

	make_page(
		o,
		area_cap+" edit",
		redirect_to([=]()->URL{
			if(a.after) return *a.after;
			Meeting_editor page;
			page.id=Meeting_id{a.meeting_id};
			return URL{to_query(page)};
		}())
	);
}

Plan historical_plan(DB db,Date date){
	auto q=qm<Date,Decimal>(
		db,
		"SELECT date,length "
		"FROM meeting_info "
		"WHERE "
			"id IN ("
				"SELECT MAX(id) FROM meeting_info "
				"WHERE DATE(edit_date)<="+escape(date)+" "
				"GROUP BY meeting_id"
			") "
			"AND valid "
			"AND date>"+escape(date)+" "
		"ORDER BY date"
	);
	return mapf(
		[](auto x){
			return make_pair(
				get<0>(x),
				Meeting_plan{get<1>(x),{}}
			);
		},
		q
	);
}

vector<Build_item> historical_to_build(DB db,Date date){
	auto q=qm<Part_id,Machine,Decimal,Subsystem_id>(
		db,
		"SELECT part_id,machine,GREATEST(time,0.1),subsystem "
		"FROM part_info "
		"WHERE "
			"id IN ("
				"SELECT MAX(id) FROM part_info "
				"WHERE DATE(edit_date)<="+escape(date)+" "
				"GROUP BY part_id"
			") "
			"AND valid "
			"AND (part_state='cut_list' OR part_state='find' OR part_state='_3d_print' OR part_state='fab') "
		"ORDER BY part_id"
	);

	vector<Build_item> r;
	for(auto row:q){
		r|=Build_item{
			get<0>(row),
			get<1>(row),
			get<2>(row),
			{},
			std::nullopt,
			0
		};
	}

	map<Subsystem_id,set<Item>> dependencies;
	for(auto [part,_1,_2,subsystem]:q){
		(void)_1;
		(void)_2;
		dependencies[subsystem].insert(part);
	}

	const auto wait_items=to_map(qm<Subsystem_id,Date>(
		db,
		"SELECT subsystem,MAX(arrival_date) "
		"FROM part_info "
		"WHERE "
			"id IN ("
				"SELECT MAX(id) FROM part_info "
				"WHERE DATE(edit_date)<="+escape(date)+" "
				"GROUP BY part_id"
			") "
			"AND valid "
			"AND (part_state='on_order')"
		"GROUP BY subsystem "
		"ORDER BY subsystem "
	));

	auto q1=qm<Subsystem_id,Decimal,optional<Subsystem_id>,Priority>(
		db,
		"SELECT subsystem_id,GREATEST(time,.1),parent,priority "
		"FROM subsystem_info "
		"WHERE "
			"id IN ("
				"SELECT MAX(id) FROM subsystem_info "
				"WHERE DATE(edit_date)<="+escape(date)+" "
				"GROUP BY subsystem_id"
			") "
			"AND valid "
			"AND (state='parts' OR state='assembly') "
		"ORDER BY subsystem_id"
	);

	for(auto [id,_1,parent,_3]:q1){
		(void)_1;
		(void)_3;
		if(parent){
			dependencies[*parent].insert(id);
		}
	}

	for(auto row:q1){
		auto id=get<0>(row);
		r|=Build_item{
			id,
			Machine::none,
			get<1>(row),
			dependencies[id],
			[=]()->std::optional<Date>{
				auto f=wait_items.find(id);
				if(f==wait_items.end()){
					return std::nullopt;
				}
				return f->second;
			}(),
			get<3>(row)
		};
	}

	//print_lines(sorted(r));
	auto find_elem=[&](Item a)->Build_item*{
		for(auto& x:r){
			if(x.item==a) return &x;
		}
		return NULL;
		/*print_lines(r);
		PRINT(a);
		assert(0);*/
	};

	std::function<void(Item,Priority)> inherit_priority;
	inherit_priority=[&](Item a,Priority priority){
		//cout<<"inherit:"<<a<<" "<<priority<<"\n";
		auto *e=find_elem(a);
		if(!e) return;
		auto& elem=*e;
		elem.priority=max(elem.priority,priority);
		if(holds_alternative<Subsystem_id>(a)){
			for(auto item:dependencies[get<Subsystem_id>(a)]){
				inherit_priority(item,elem.priority);
			}
		}
		//cout<<"done\n";
	};

	for(auto [parent,_]:dependencies){
		(void)_;
		inherit_priority(Item{parent},Priority{0});
	}

	return r;
}

set<Date> days_with_changes(DB db,Table_name const& table){
	auto q=qm<Date>(
		db,
		"SELECT DISTINCT(DATE(edit_date)) "
		"FROM "+table+" "
		"ORDER BY DATE(edit_date)"
	);
	return to_set(mapf([](auto x){ return get<0>(x); },q));
}

set<Date> days_with_changes(DB db){
	return (
		days_with_changes(db,"part_info")|
		days_with_changes(db,"subsystem_info")|
		days_with_changes(db,"meeting_info")
	);
}

Date last_date_needed(Plan plan){
	Date last{};//initialized to 0-0-0
	for(auto [date,meeting_plan]:plan){
		if(meeting_plan.to_do.size()){
			last=max(last,date);
		}
	}
	return last;
}

Date date_needed_for(Plan plan,Item item){
	//On what day does part/asm X get finished?
	Date last{};
	for(auto const& [date,meeting_plan]:plan){
		for(auto const& [machine,item_parts]:meeting_plan.to_do){
			(void)machine;
			for(auto const& [length,item_here]:item_parts){
				(void)length;
				if(item==item_here){
					last=max(last,date);
				}
			}
		}
	}
	return last;
}

void timeline(DB db){
	//at each date, figure out when we think we're going to be done.

	/*Date test_date{2019,12,8};
	{
		auto tb=to_build(db);
		auto tbh=historical_to_build(db,test_date);

		auto sc=to_set(tb);
		auto sh=to_set(tbh);

		cout<<"Current only:\n";
		print_lines(sc-sh);

		cout<<"Hist only:\n";
		print_lines(sh-sc);
		
		cout<<"Diff:\n";
		diff(tb,tbh);
		assert(tb==tbh);
	}

	{
		auto planh=historical_plan(db,test_date);
	}*/

	for(auto date:days_with_changes(db)){
		auto plan=historical_plan(db,date);
		auto to_build=historical_to_build(db,date);
		//print_lines(to_build);
		auto s=schedule(plan,to_build);
		//PRINT(s);
		//print_r(s);
		cout<<date<<":";
		if(s.second.size()){
			cout<<"Not completable\n";
		}else{
			cout<<last_date_needed(s.first)<<"\n";
		}
		cout<<"\t"<<date_needed_for(s.first,Item{Subsystem_id{535}})<<"\n";
	}
}

vector<pair<Date,Date>> skip_unchanged_second(vector<pair<Date,Date>> const& a){
	if(a.empty()) return {};
	vector<pair<Date,Date>> r;
	r|=a[0];
	for(auto [v1,v2]:tail(a)){
		if(v2!=r[r.size()-1].second){
			r|=make_pair(v1,v2);
		}
	}
	return r;
}

string completion_est(DB db,Item item){
	vector<pair<Date,Date>> r;
	for(auto date:days_with_changes(db)){
		auto plan=historical_plan(db,date);
		auto to_build=historical_to_build(db,date);
		auto s=schedule(plan,to_build);
		auto sched=date_needed_for(s.first,item);
		r|=make_pair(date,sched);
	}
	r=skip_unchanged_second(r);
	stringstream ss;
	ss<<h2("Completion estimates");
	ss<<"<table border>";
	ss<<tr(th("Date of estimate")+th("Estimated completion"));
	for(auto a:r){
		ss<<tr(td(as_string(a.first))+td(as_string(a.second)));
	}
	ss<<"</table>";
	return ss.str();
}

string completion_est(DB db,Part_id const& part){
	return completion_est(db,Item{part});
}

string completion_est(DB db,Subsystem_id const& subsystem){
	return completion_est(db,Item{subsystem});
}
