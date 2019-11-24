#include "subsystems.h"
#include "home.h"

using namespace std;

Dummy& operator+=(Dummy& a,Dummy){
	return a;
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

string show_table(DB db,Request const& page,Table_name const& name,optional<string> title){
	auto columns=mapf([](auto x){ return *x; },firsts(query(db,"DESCRIBE "+name)));
	stringstream ss;
	ss<<h2(title?*title:name);
	auto q=query(db,"SELECT * FROM "+name);
	ss<<make_table(page,columns,q);
	return ss.str();
}

Machine_page make_machine_page(Machine a){
	Machine_page r;
	r.machine=a;
	return r;
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

string show_current_subsystems(DB db,Request const& page){
	return as_table(
		db,
		page,
		vector<Label>{"Name","Prefix","Parent"},
		qm<Subsystem_id,optional<Subsystem_prefix>,optional<Subsystem_id>>(
			db,
			"SELECT subsystem_id,prefix,parent "
			"FROM subsystem_info "
			"WHERE "
				"valid AND "
				"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id)"
		)
	);
}

string subsystem_machine_count(DB db,Request const& page){
	auto q=qm<
		variant<Subsystem_id,string>,
		#define X(A) optional<int>,
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

