#include "subsystem.h"
#include "home.h"
#include "subsystems.h"
#include "part.h"
#include "meeting.h"

using namespace std;

string current_user(){
	{
		//In normal operation, this should always be set
		char *s=getenv("REMOTE_USER");
		if(s) return s;
	}
	{
		//When running on dev server w/o user auth enabled
		char *s=getenv("SERVER_NAME");
		if(s) return s;
	}
	//When running directly from command line
	return "no_user";
}

Id new_item(DB db,Table_name const& table){
	run_cmd(db,"INSERT INTO "+table+" VALUES ()");
	auto q=query(db,"SELECT LAST_INSERT_ID()");
	//PRINT(q);
	assert(q.size()==1);
	assert(q[0].size()==1);
	assert(q[0][0]);
	auto id=stoi(*q[0][0]);
	return id;
}

void inner(ostream& o,Subsystem_new const& a,DB db){
	auto id=new_item(db,"subsystem");

	if(a.parent){
		auto q="INSERT INTO subsystem_info ("
			"subsystem_id,"
			"edit_user,"
			"edit_date,"
			"valid,"
			"parent "
			") VALUES ("
			+escape(id)+","
			+escape(current_user())+","
			"now(),"
			"1,"
			+escape(a.parent)
			+")";
		run_cmd(db,q);
	}
	Subsystem_editor page;
	page.id={id};
	make_page(
		o,
		"New subsystem",
		redirect_to(page)
	);
}

string parts_of_subsystem(DB db,Request const& page,Subsystem_id id){
	auto q=qm<Part_id,Part_state,int>(
		db,
		"SELECT part_id,part_state,qty "
		"FROM part_info "
		"WHERE (id) IN "
			"(SELECT MAX(id) FROM part_info GROUP BY part_id) "
			"AND valid AND subsystem="+escape(id)
	);
	return h2("Parts directly in subsystem")+as_table(db,page,vector<Label>{"Part","State","Qty"},q);
}

string subsystems_of_subsystem(DB db,Request const& page,Subsystem_id subsystem){
	return h2("Subsystems directly in this subsystem")+as_table(
		db,
		page,
		vector<Label>{"Subsystem","Prefix"},
		qm<Subsystem_id,Subsystem_prefix>(
			db,
			"SELECT subsystem_id,prefix "
			"FROM subsystem_info "
			"WHERE "
				"valid AND "
				"id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) AND "
				"parent="+escape(subsystem)
		)
	);
}

Subsystem_data subsystem_data(DB db,Subsystem_id id){
	vector<string> data_col_names;
	#define X(A,B) data_col_names|=""#B;
	SUBSYSTEM_DATA(X)
	#undef X

	auto q=query(
		db,
		"SELECT "+join(",",data_col_names)+
		" FROM subsystem_info "
		"WHERE subsystem_id="+escape(id)+
		" ORDER BY edit_date DESC LIMIT 1"
	);
	Subsystem_data current{};
	if(q.size()==0){
		current.valid=1;
	}else{
		assert(q.size()==1);
		auto row=q[0];
		unsigned i=0;
		#define X(A,B) { \
			if(row[i]) current.B=parse((A*)0,*row[i]); \
			i++;\
		}
		SUBSYSTEM_DATA(X)
		#undef X
	}

	return current;
}

void inner(ostream& o,Subsystem_editor const& a,DB db){
	vector<string> info_col_names;
	#define X(A,B) info_col_names|=""#B;
	SUBSYSTEM_INFO_ROW(X)
	#undef X

	auto current=subsystem_data(db,a.id);
	make_page(
		o,
		as_string(current.name)+" Subsystem",
		string()+"<form>"
		"<input type=\"hidden\" name=\"p\" value=\"Subsystem_edit\">"
		"<input type=\"hidden\" name=\"subsystem_id\" value=\""+escape(a.id)+"\">"+
		input_table([=](){
			vector<Input> r;
			#define X(A,B) r|=show_input(db,""#B,current.B);
			SUBSYSTEM_DATA(X)
			#undef X
			return r;
		}())+
		"<br><input type=\"submit\">"+
		"</form>"
		+link([=](){ BOM r; r.subsystem=a.id; return r; }(),"BOM")
		+" "+link([=](){ Subsystem_duplicate r; r.subsystem=a.id; return r; }(),"Duplicate")
		+h2("Overview")
		+[=](){
			stringstream ss;
			Subsystem_new sub_new;
			sub_new.parent=a.id;
			ss<<link(sub_new,"New Subsystem")<<" ";
			Part_new part_new;
			part_new.subsystem=a.id;
			ss<<link(part_new,"New Part");
			return ss.str();
		}()
		+"<table border><tr>"+th("Name")+th("Status")+"</tr>"+indent_sub_table(db,0,a.id,{})+"</table>"
		+parts_of_subsystem(db,a,a.id)
		+subsystems_of_subsystem(db,a,a.id)
		+h2("History")
		+make_table(
			a,
			{
				//"edit_date","edit_user","id","name","subsystem_id","valid","parent","prefix"
				#define X(A,B) ""#B,
				SUBSYSTEM_INFO_ROW(X)
				#undef X
			},
			query(
				db,
				"SELECT "+join(",",info_col_names)+
				" FROM subsystem_info WHERE subsystem_id="+escape(a.id)
			)
		)
	);
}

void inner(ostream& o,Subsystem_edit const& a,DB db){
	vector<pair<string,string>> v;
	v|=pair<string,string>("edit_date","now()");
	v|=pair<string,string>("edit_user",escape(current_user()));
	#define X(A,B) if(""#B==string("part_number")) v|=part_entry(db,a.parent,a.B); else v|=pair<string,string>(""#B,escape(a.B));
	SUBSYSTEM_EDIT_ITEMS(X)
	#undef X
	auto q="INSERT INTO subsystem_info ("
		+join(",",firsts(v))
		+") VALUES ("
		+join(",",seconds(v))
		+")";
	//PRINT(q);
	run_cmd(db,q);
	Subsystem_editor page;
	page.id=Subsystem_id{a.subsystem_id};
	make_page(
		o,
		"Subsystem edit",
		redirect_to(page)
	);
}

bool make_duplicate(DB db,Subsystem_id new_parent,Part_id current){
	//basically, just do the same thing as in the part duplicate page, except set the parent to something different
	auto data=part_data(db,current);
	if(!data){
		return 1;
	}
	auto id=Part_id{new_item(db,"part")};
	data->subsystem=new_parent;
	insert_part_data(db,id,*data);
	return 0;
}

void insert_subsystem_data(DB db,Subsystem_id id,Subsystem_data data){
	vector<pair<string,string>> items;
	items|=pair<string,string>("edit_user",escape(current_user()));
	items|=pair<string,string>("edit_date","now()");
	items|=pair<string,string>("subsystem_id",escape(id));
	#define X(A,B) items|=pair<string,string>(""#B,escape(data.B));
	SUBSYSTEM_DATA(X)
	#undef X
	stringstream ss;
	ss<<"INSERT INTO subsystem_info (";
	ss<<join(",",firsts(items));
	ss<<") VALUES (";
	ss<<join(",",seconds(items));
	ss<<")";
	run_cmd(db,ss.str());
}

Subsystem_id make_duplicate_local(DB db,optional<Subsystem_id> new_parent,Subsystem_id current){
	auto data=subsystem_data(db,current);
	auto id=Subsystem_id{new_item(db,"subsystem")};
	data.parent=new_parent;
	insert_subsystem_data(db,id,data);
	return id;
}

vector<Subsystem_id> subsystems_of(DB db,Subsystem_id a){
	return q1<Subsystem_id>(
		db,
		"SELECT subsystem_id "
		"FROM subsystem_info "
		"WHERE "
			"valid "
			"AND id IN (SELECT MAX(id) FROM subsystem_info GROUP BY subsystem_id) "
			"AND parent="+escape(a)
	);
}

vector<Part_id> parts_of(DB db,Subsystem_id a){
	return q1<Part_id>(
		db,
		"SELECT part_id "
		"FROM part_info "
		"WHERE "
			"valid "
			"AND id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
			"AND subsystem="+escape(a)
	);
}

Subsystem_id make_duplicate(DB db,optional<Subsystem_id> new_parent,Subsystem_id current,set<Subsystem_id> visited){
	//returns the id of the copy.

	if(visited.count(current)){
		throw "Error: Loop detected.  Not duplicating.";
	}
	visited|=current;

	//cout<<"Current:"<<current<<"\n";
	//first, duplicate the local data for this subsystem
	auto new_subsystem=make_duplicate_local(db,new_parent,current);
	//cout<<"New:"<<new_subsystem<<"\n";

	//next, recurse into each of the child susbsystems
	for(auto x:subsystems_of(db,current)){
		//cout<<"Rec into:"<<x<<"\n";
		make_duplicate(db,new_subsystem,x,visited);
	}

	//finally, duplicate parts
	bool error=0;
	for(auto x:parts_of(db,current)){
		//cout<<"Copy part:"<<x<<"\n";
		error|=make_duplicate(db,new_subsystem,x);
	}

	return new_subsystem;
}

optional<Subsystem_id> get_parent(DB db,Subsystem_id id){
	auto q=qm<optional<Subsystem_id>>(
		db,
		"SELECT parent "
		"FROM subsystem_info "
		"WHERE "
			"valid "
			"AND id IN (SELECT MAX(id) FROM subsystem_info WHERE subsystem_id="+escape(id)+")"
	);
	if(q.size()==0) return std::nullopt;
	assert(q.size()==1);
	return get<0>(q[0]);
}

void inner(ostream& o,Subsystem_duplicate const& a,DB db){
	//duplicate depth-first
	//first, check that the given subsystem exists
	//next, duplicate 
	auto parent=get_parent(db,a.subsystem);
	auto new_id=make_duplicate(db,parent,a.subsystem,{});

	make_page(
		o,
		"Subsystem duplicate",
		redirect_to([=](){
			Subsystem_editor r;
			r.id=new_id;
			return r;
		}())
	);
}
