#include "part.h"
#include "subsystem.h"
#include "home.h"
#include "subsystems.h"

using namespace std;

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

Subsystem_prefix get_prefix(DB db,Subsystem_id subsystem){
	auto q=qm<Subsystem_prefix>(
		db,
		"SELECT prefix "
		"FROM subsystem_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) "
			"AND valid "
			"AND subsystem_id="+escape(subsystem)
	);
	assert(q.size()==1);
	return get<0>(q[0]);
}

string escape(Part_number_local a){
	stringstream ss;
	ss<<"'"<<a<<"'";
	return ss.str();
}

template<typename T>
pair<string,string> part_entry(DB db,optional<Subsystem_id> subsystem_id,Part_state state,T t){
	if( (as_string(t)=="" || as_string(t)=="NULL") && subsystem_id ){
		//give it the next free part number for whatever assembly it's in.
		auto prefix=get_prefix(db,*subsystem_id);
		auto q=qm<optional<Part_number>>(
			db,
			"SELECT MAX(part_number) FROM part_info WHERE part_number REGEXP '"+as_string(prefix)+"[0-9][0-9][0-9]-1425-2020'"
		);
		PRINT(q);
		assert(q.size()==1);
		if(get<0>(q[0])){
			Part_number_local a(*get<0>(q[0]));
			return make_pair("part_number",escape(next(a)));
		}else{
			return make_pair("Part_number","'"+as_string(prefix)+"000-1425-2020"+"'");
		}
	}
	return make_pair("part_number",escape(t));
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
		#define X(A,B) [&]()->string{ \
			if(should_show(current.part_state,""#B)) \
				return show_input(db,""#B,current.B); \
			else \
				return ""; \
		}()+
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

void inner(std::ostream& o,Part_edit const& a,DB db){
	string area_lower="part";
	string area_cap="Part";

	vector<pair<string,string>> v;
	v|=pair<string,string>("edit_date","now()");
	v|=pair<string,string>("edit_user",escape(current_user()));
	#define X(A,B) if(""#B==string("part_number")) v|=part_entry(db,a.subsystem,a.part_state,a.B); else v|=pair<string,string>(""#B,escape(a.B));
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

