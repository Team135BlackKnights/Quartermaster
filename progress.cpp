#include "progress.h"
#include<fstream>
#include "util.h"
#include "queries.h"
#include "home.h"
#include "meeting.h"

using namespace std;

//start generic code

template<typename Func,typename T>
auto group(Func f,vector<T> a){
	using K=decltype(f(a[0]));
	map<K,vector<T>> r;
	for(auto elem:a){
		r[f(elem)]|=elem;
	}
	return r;
}

template<typename T>
T head(vector<T> a){
	return a.at(0);
}

template<typename T>
map<T,size_t> count(vector<T> const& a){
	map<T,size_t> r;
	for(auto elem:a){
		r[elem]++;
	}
	return r;
}

//start program-specific code

struct Part_info_row{
	PART_INFO_ROW(INST)
};

string diff(Part_info_row const& a,Part_info_row const& b){
	vector<pair<string,pair<string,string>>> r;
	#define X(A,B) if(a.B!=b.B && ""#B!=string("edit_date") && ""#B!=string("id")) r|=pair<string,pair<string,string>>(""#B,make_pair(as_string(a.B),as_string(b.B)));
	PART_INFO_ROW(X)
	#undef X
	return join("\n",MAP(as_string,r));
}

void part_diff(DB db){
	//First, just show history as diffs.
	vector<string> columns;
	#define X(A,B) columns|=""#B;
	PART_INFO_ROW(X)
	#undef X
	auto q=query(
		db,
		"SELECT "+join(",",columns)+" FROM part_info ORDER BY edit_date LIMIT 30"
	);
	vector<Part_info_row> parts;
	for(auto row:q){
		Part_info_row a;
		size_t i=0;
		#define X(A,B) { if(row[i]) a.B=parse((A*)0,*row[i]); i++; }
		PART_INFO_ROW(X)
		#undef X
		parts|=a;
	}

	auto by_part=group([](auto x){ return x.part_id; },parts);
	for(auto [part,data]:by_part){
		cout<<part<<"\n";
		cout<<data.size()<<"\n";
		auto start=head(data);
		PRINT(start.edit_date);
		//auto date=get<3>(start);
		//auto name=
		//PRINT(date);
		//PRINT(start);

		cout<<"Create:"<<start.name<<"\n";
		for(auto elem:tail(data)){
			cout<<"\t"<<diff(start,elem)<<"\n";
			cout<<"\n";
			start=elem;
		}
		cout<<"\n";
	}
}

/*void subsystem_diff(DB db){
	
}*/

//void meeting_diff(DB);

void in_state_by_date(DB db){
	//to start with, only look at parts; ignore subsystems
	//first, decide on what dates are interesting
	//to start with, could just make it anytime anything changes
	map<Part_id,Part_state> now;
	using Total=map<Part_state,size_t>;
	auto get_total=[&](){
		return count(values(now));
	};

	vector<pair<Datetime,Total>> timeline;

	auto q=qm<Datetime,Part_id,Part_state>(
		db,
		"SELECT edit_date,part_id,part_state "
		"FROM part_info "
		"WHERE valid "
		"ORDER BY id"
	);

	for(auto [edit_date,part,state]:q){
		if(now[part]!=state){
			now[part]=state;
			timeline|=make_pair(edit_date,get_total());
		}
	}

	//print_lines(timeline);
	PRINT(timeline.size());

	//output as CSV
	ofstream o("timeline.csv");
	o<<"date,";
	#define X(A) o<<""#A<<",";
	PART_STATES(X)
	#undef X
	o<<"\n";

	for(auto [date,data]:timeline){
		o<<date<<",";
		#define X(A) o<<data[Part_state::A]<<",";
		PART_STATES(X)
		#undef X
		o<<"\n";
	}
}

void left_by_date(DB db){
	//would also want this by machine
	//this is time estimates
	//schedule finish by date
}

void progress(DB db){
	//part_diff(db);
	//next, try to make a version of the scheduler which will run on data only from specific date ranges
	//or, make the listing of how many items are in what state by date
	//in_state_by_date(db);

	timeline(db);

	//would be nice to have a page where given two dates you get a listing of what parts have change state, etc.
}

map<Part_id,Part_state> part_states(DB db,optional<Date> date){
	auto q=qm<Part_id,Part_state>(
		db,
		"SELECT part_id,part_state FROM part_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM part_info "
			+[=]()->string{
				if(date){
					return "WHERE DATE(edit_date)<"+escape(date);
				}
				return "";
			}()
			+" GROUP BY part_id) "
			"AND valid"
	);
	map<Part_id,Part_state> r;
	for(auto [id,state]:q) r[id]=state;
	return r;
}

string change_table(State_change const& a,DB db){
	/*auto q=qm<Part_id,Part_state,Valid,Part_state,Valid>(
		db,
		"SELECT a.part_id,a.part_state,a.valid,b.part_state,b.valid "
		"FROM part_info a,part_info b "
		"WHERE "
			"a.id IN (SELECT MAX(id) FROM part_info WHERE DATE(edit_date)<"+escape(a.start)+" GROUP BY part_id) "
			"AND b.id IN (SELECT MAX(id) FROM part_info WHERE DATE(edit_date)<="+escape(a.end)+" GROUP BY part_id) "
			"AND a.part_id=b.part_id "
			"AND (a.part_state!=b.part_state OR a.valid!=b.valid) "
		"ORDER BY b.part_state"
	);*/

	/*auto q2=qm<Part_id,Part_state>(
		db,
		"SELECT " "FROM part_info WHERE "
	);*/

	map<Part_id,pair<optional<Part_state>,optional<Part_state>>> m;
	for(auto [id,state]:part_states(db,a.start)){
		m[id].first=state;
	}
	for(auto [id,state]:part_states(db,a.end)){
		m[id].second=state;
	}
	vector<tuple<Part_id,std::optional<Part_state>,std::optional<Part_state>>> r;
	for(auto [k,v]:m){
		if(v.first!=v.second){
			r|=make_tuple(k,v.first,v.second);
		}
	}

	return as_table(db,a,{"Part","Old state","New state"},r)+" Warning: Showing only part changes, not assemblies.";
	//todo: put in basically the same logic for the subsystem and the meeting tables.
}

void inner(std::ostream& o,State_change const& a,DB db){
	make_page(
		o,
		"State change "+as_string(a.start)+" to "+as_string(a.end),
		"<form>" //this might make sense to have auto-submit on any change
		"<input type=\"hidden\" name=\"p\" value=\"State_change\">"
		"<input type=\"hidden\" name=\"sort_by\" value="+escape(a.sort_by)+">"
		"<input type=\"hidden\" name=\"sort_order\" value="+escape(a.sort_order)+">"
		+"<input type=\"date\" name=\"start\" value="+escape(a.start)+" onchange=\"this.form.submit();\">"
		+"<input type=\"date\" name=\"end\" value="+escape(a.end)+" onchange=\"this.form.submit();\">"
		+"</form>"
		+change_table(a,db) //part#/asm,old state,new state
	);
}
