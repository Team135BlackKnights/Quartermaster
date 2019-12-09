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
}

