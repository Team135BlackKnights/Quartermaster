#include "part.h"
#include "subsystem.h"
#include "home.h"
#include "subsystems.h"
#include "meeting.h"
#include "parts.h"

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

Part_number_local next_part_number(DB db,Subsystem_id subsystem){
	auto prefix=get_prefix(db,subsystem);
	auto from_table=[&](string table)->optional<Part_number_local>{
		auto q=qm<optional<Part_number>>(
			db,
			"SELECT MAX(part_number) FROM "+table+" WHERE part_number REGEXP '"+as_string(prefix)+"[0-9][0-9][0-9]-1425-2020'"
		);
		assert(q.size()==1);
		auto found=get<0>(q[0]);
		if(!found) return std::nullopt;
		return Part_number_local(*found);
	};

	auto latest=max(from_table("part_info"),from_table("subsystem_info"));
	if(latest) return next(*latest);
	return Part_number_local{get_prefix(db,subsystem),Three_digit{0}};
}

bool should_show(Part_state state,string const& name){
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

string fields_by_state(){
	stringstream o;
	o<<h2("Part fields by state");
	o<<"<table border>";
	o<<"<tr>";
	o<<td("");
	for(auto state:options((Part_state*)0)){
		o<<th(as_string(state));
	}
	o<<"</tr>";

	vector<string> items;
	#define X(A,B) items|=""#B;
	PART_DATA(X)
	#undef X
	
	for(auto item:items){
		o<<"<tr>";
		o<<th(item);
		for(auto state:options((Part_state*)0)){
			o<<td(as_string(should_show(state,item)));
		}
		o<<"</tr>";
	}
	o<<"</table>";
	return o.str();
}

string after_done(){
	auto g1=getenv("HTTP_REFERER");
	auto p=parse_referer(g1);
	if(!p) return "";
	return "<input type=\"hidden\" name=\"after\" value=\""+to_query(*p)+"\">";
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
		"WHERE "+area_lower+"_id="+escape(a.id)+
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
		as_string(current.name)+" (part)",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\""+area_cap+"_edit\">"
		"<input type=\"hidden\" name=\""+area_lower+"_id\" value=\""+escape(a.id)+"\">"+
		after_done()+
		input_table([=](){
			vector<Input> r;
			/*#define X(A,B) \
				if(should_show(current.part_state,""#B)) \
					r|=show_input(db,""#B,current.B);
			PART_DATA(X)
			#undef X*/
			#define X(A,B) r|=show_input(db,""#B,current.B);
			PART_DATA(X)
			#undef X
			return r;
		}())+
		"<br><input type=\"submit\">"+
		"</form>"
		+link(
			[a](){
				Part_duplicate r;
				r.part=a.id;
				return r;
			}(),
			"Duplicate"
		)
		+completion_est(db,a.id)
		+h2("History")
		+make_table(
			a,
			all_cols,
			query(db,"SELECT "+join(",",all_cols)+" FROM "+area_lower+"_info WHERE "+area_lower+"_id="+escape(a.id))
		)
		+viz_func()
	);
}

string redirect_to(URL const& url){
	//go parse it out, which is a bit silly.
	try{
		auto p=parse_query(url.data);
		return redirect_to(p);
	}catch(string const& s){
		return "Would redirect, but does not appear to be a valid page:"+as_string(url);
	}
}

void inner(std::ostream& o,Part_edit const& a,DB db){
	string area_lower="part";
	string area_cap="Part";

	vector<pair<string,string>> v;
	v|=pair<string,string>("edit_date","now()");
	v|=pair<string,string>("edit_user",escape(current_user()));
	#define X(A,B) if(""#B==string("part_number")) v|=part_entry(db,a.subsystem,a.B); else v|=pair<string,string>(""#B,escape(a.B));
	PART_EDIT_DATA_ITEMS(X)
	#undef X
	auto q="INSERT INTO "+area_lower+"_info ("
		+join(",",firsts(v))
		+") VALUES ("
		+join(",",seconds(v))
		+")";
	run_cmd(db,q);
	make_page(
		o,
		area_cap+" edit",
		redirect_to([=]()->URL{
			if(a.after) return *a.after;
			Part_editor page;
			page.id=Part_id{a.part_id};
			return URL{to_query(page)};
		}())
	);
}

optional<Part_data> part_data(DB db,Part_id part){
	auto q=query(
		db,
		"SELECT "
		#define X(A,B) ""#B ","
		PART_DATA_INNER(X)
		#undef X
		"0 FROM part_info "
		"WHERE "
			"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
			"AND valid "
			"AND part_id="+escape(part)
	);
	if(q.size()!=1){
		return std::nullopt;
	}
	
	//explicitly initialize to avoid undefined data in null fields.
	Part_data data{};
	data.valid=1;
	{
		auto row=q[0];
		unsigned i=0;
		#define X(A,B) { if(row[i]) data.B=parse((A*)0,*row[i]); i++; }
		PART_DATA_INNER(X)
		#undef X
	}
	return data;
}

void insert_part_data(DB db,Part_id part_id,Part_data const& data){
	run_cmd(
		db,
		[=](){
			vector<pair<string,string>> items;
			items|=pair<string,string>("part_id",escape(part_id));
			items|=pair<string,string>("edit_user",escape(current_user()));
			items|=pair<string,string>("edit_date","now()");
			#define X(A,B) items|=pair<string,string>(""#B,escape(data.B));
			PART_DATA(X)
			#undef X
	
			//create part_info entry from existing data
			stringstream ss;
			ss<<"INSERT INTO part_info (";
			ss<<join(",",firsts(items));
			ss<<") VALUES (";
			ss<<join(",",seconds(items));
			ss<<")";
			return ss.str();
		}()
	);
}

void inner(std::ostream& o,Part_duplicate const& a,DB db){
	auto data=part_data(db,a.part);
	if(!data){
		make_page(
			o,
			"Part duplicate",
			"Error: Could not find information about part to duplicate.  Part id:"+as_string(a.part)
		);
		return;
	}
	
	//create part
	auto id=Part_id{new_item(db,"part")};
	insert_part_data(db,id,*data);
	make_page(
		o,
		"Part_duplicate",
		redirect_to([=](){
			Part_editor p;
			p.id=id;
			return p;
		}())
	);
}

void inner(std::ostream& o,Batch_entry const& a,DB db){
	make_page(
		o,
		"Batch part entry",
		[=](){
			stringstream o;
			o<<"<form>";
			o<<"<input type=\"hidden\" name=\"p\" value=\"Batch_entry_backend\">\n";
			o<<"<table>";
			o<<tr(th("Subsystem")+th("Part name")+th("Qty"));
			for(auto i:range(5)){
				vector<Input> inputs{
					show_input(db,"subsystem"+as_string(i),Subsystem_id{}),
					show_input(db,"name"+as_string(i),string{}),
					show_input(db,"qty"+as_string(i),unsigned(0))
				};
				o<<input_table(inputs)<<"<p></p>";
			}
			o<<"</table>";
			o<<"<input type=\"submit\">";
			o<<"</form>";
			return o.str();
		}()
	);
}

void inner(std::ostream& o,Batch_entry_backend const& a,DB db){
	vector<Entry> data;
	#define PARSE(N) if(a.name##N.size()){\
		Entry e;\
		e.subsystem=a.subsystem##N;\
		e.name=a.name##N;\
		e.qty=a.qty##N;\
		data|=e;\
	}
	PARSE(0)
	PARSE(1)
	PARSE(2)
	PARSE(3)
	PARSE(4)

	for(auto entry:data){
		auto id=new_item(db,"part");
		auto q="INSERT INTO part_info ("
			"part_id,"
			"edit_user,"
			"edit_date,"
			"valid,"
			"subsystem,"
			"name,"
			"qty "
			") VALUES ("
			+escape(id)+","
			+escape(current_user())+","
			"now(),"
			"1,"
			+escape(entry.subsystem)+","
			+escape(entry.name)+","
			+escape(entry.qty)
			+")";
		run_cmd(db,q);
	}

	Batch_entry page;
	make_page(
		o,
		"Batch entry backend",
		"Not impl:"+as_string(a)+"<p>"+as_string(data.size())+" "+as_string(data) //redirect_to(page)
	);
}
