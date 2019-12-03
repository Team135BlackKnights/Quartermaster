#include "meeting.h"
#include "subsystems.h"
#include "subsystem.h"

using namespace std;

template<typename K,typename V>
vector<V> values(map<K,V> const& a){
	vector<V> r;
	for(auto [_,v]:a){
		(void)_;
		r|=v;
	}
	return r;
}

template<typename T>
vector<T> flatten(vector<vector<T>> const& a){
	vector<T> r;
	for(auto elem:a){
		r|=elem;
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

std::string show_plan(DB db,Request const& page);

std::string input_table(vector<Input> const& a){
	stringstream ss;
	ss<<"<table>";
	for(auto elem:a){
		ss<<"<tr>";
		ss<<"<td align=right>"<<elem.name<<"</td>";
		ss<<td(elem.form)<<td(elem.notes);
		ss<<"</tr>";
	}
	ss<<"</table>";
	return ss.str();
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
		as_string(current.date)+" (meeting)",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\""+area_cap+"_edit\">"
		"<input type=\"hidden\" name=\""+area_lower+"_id\" value=\""+as_string(a.id)+"\">"
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
	make_page(
		o,
		"Calendar",
		current_calendar(db,a)
		+to_do(db,a)
		+show_plan(db,a)
		+history_meeting(db,a)//+show_table(db,a,"meeting_info","History")
	);
}

/*struct Plan{
	m
};*/
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
	X(Decimal,length)

struct Build_item{
	BUILD_ITEMS(INST)
};

std::ostream& operator<<(std::ostream& o,Build_item const& a){
	o<<"Build_item( ";
	#define X(A,B) o<<""#B<<":"<<a.B<<" ";
	BUILD_ITEMS(X)
	#undef X
	return o<<")";
}

vector<Build_item> to_build(DB db){
	auto q=qm<Part_id,Machine,Decimal>(
		db,
		"SELECT part_id,machine,time "
		"FROM part_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
			"AND valid "
			"AND (part_state='cut_list' OR part_state='find' OR part_state='_3d_print' OR part_state='fab') "
	);

	vector<Build_item> r;
	for(auto row:q){
		r|=Build_item{get<0>(row),get<1>(row),get<2>(row)};
	}

	auto q1=qm<Subsystem_id,Decimal>(
		db,
		"SELECT subsystem_id,time "
		"FROM subsystem_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) "
			"AND valid "
			"AND (state='parts' OR state='assembly')"
	);
	for(auto row:q1){
		r|=Build_item{get<0>(row),Machine::none,get<1>(row)};
	}
	return r;
}

Decimal machine_time_left(Meeting_plan mp,Machine machine){
	/*auto total_time_used=[=](){
		Decimal r=0;
		for(auto [k,v]:mp.to_do){
			for(auto [item,length]:v){
				nyi//r+=length;
			}
		}
		return r;
	}();*/
	auto total_time_used=sum(
		firsts(flatten(values(mp.to_do)))
	);
	
	auto overall_time_available=mp.length*3-total_time_used;
	
	auto machine_time_used=sum(firsts(mp.to_do[machine]));
	auto machine_time_available=mp.length-machine_time_used;

	return min(overall_time_available,machine_time_available);
}

pair<Plan,string> make_plan_inner(DB db){
	//this is going to be a graph problem...
	//if you cared that much about it

	//first, going to just schedule building the parts and not care about assembly.
	auto plan=blank_plan(db);
	//print_lines(plan);
	auto to_schedule=to_build(db);
	//print_lines(to_schedule);

	auto schedule_part=[&](auto &x)->std::optional<std::string>{
		for(auto &meeting:plan){
			auto m=machine_time_left(meeting.second,x.machine);
			if(m>0){
				auto to_use=min(m,x.length);
				x.length-=to_use;
				meeting.second.to_do[x.machine]|=make_pair(to_use,x.item);
				if(x.length==0) return std::nullopt;
			}
		}
		stringstream ss;
		ss<<"Planned so far:\n";
		//print_lines(plan);
		for(auto elem:plan) ss<<elem<<"\n";
		ss<<"Could not schedule item.  Out of meetings.\n";
		ss<<"Attempting to schedule:"<<x<<"\n";
		return ss.str();
	};

	//TODO: Put dependencies in here.
	for(auto x:to_schedule){
		//PRINT(x);
		while(x.length>0){
			auto s=schedule_part(x);
			if(s){
				return make_pair(plan,*s);
			}
		}
	}
	return make_pair(plan,string{});
}

template<typename T>
string join(vector<T> const& v){
	return join("",v);
}

auto li(std::string s){ return tag("li",s); }

std::string show_plan(DB db,Request const& page){
	auto x=make_plan_inner(db);
	stringstream o;
	o<<h2("Scheduled tasks");
	o<<"Methodology notes:\n";
	o<<"<ul>";
	o<<li("Items that are in still in design are totally ignored.");
	o<<li("Items that are to be ordered are totally ignored");
	o<<li("Items with time taken listed as 0 are totally ignored.");
	o<<li("Dependencies between parts and their assemblies are ignored.");
	o<<li("Assumes that the amount of time for any given machines is equal to meeting length and overall time is 3x meeting length");
	o<<"</ul>";
	o<<"<table border>";
	o<<tr(join("",mapf(th,vector{"Date","Length","Notes","To do"})));
	for(auto a:x.first){
		o<<"<tr>";
		o<<td(as_string(a.first));
		auto mp=a.second;
		o<<td(as_string(mp.length));
		o<<td("");//TODO: Fill this in
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
	o<<x.second;
	return o.str();
}

void make_plan(DB db){
	auto x=make_plan_inner(db);
	print_lines(x.first);
	cout<<x.second<<"\n";
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

