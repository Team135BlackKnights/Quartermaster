#include "order.h"
#include "home.h"
#include "subsystem.h"

using namespace std;

string to_order(DB db,Request const& page){
	stringstream ss;
	ss<<h2("To order");
	ss<<"<form>";
	ss<<"<input type=\"hidden\" name=\"p\" value=\"Order_edit\">";
	ss<<as_table(
		db,
		page,
		vector<Label>{"Subsystem","Part","Supplier","Part #","qty","part_link","price","In new order"},
		qm<Subsystem_id,Part_id,Supplier,Part_number,unsigned,URL,Decimal,Part_checkbox>(
			db,
			"SELECT subsystem,part_id,part_supplier,part_number,qty,part_link,price,part_id "
			"FROM part_info "
			"WHERE "
				"valid "
				"AND id IN (SELECT max(id) FROM part_info GROUP BY part_id) "
				"AND part_state='buy_list'"
		)
	);
	ss<<"<br>New order: Arrival date:<input type=\"date\" name=\"arrival_date\">";
	ss<<" <input type=\"submit\" value=\"Order made\">";
	ss<<"</form>";
	return ss.str();
}

struct Arrived_button:Wrap<Arrived_button,Part_id>{};

std::string pretty_td(DB,Arrived_button a){
	return td([=](){
		stringstream ss;
		ss<<"<form>";
		ss<<"<input type=\"hidden\" name=\"p\" value=\"Arrived\">";
		ss<<"<input type=\"hidden\" name=\"part\" value=\""<<as_string(a)<<"\">";
		ss<<"<input type=\"submit\" value=\"Arrived\">";
		ss<<"</form>";
		return ss.str();
	}());
}

string on_order(DB db,Request const& page){
	return h2("On order")+as_table(
		db,
		page,
		vector<Label>{"Subsystem","Part","Supplier","Part #","qty","Expected arrival","Update"},
		qm<Subsystem_id,Part_id,Supplier,Part_number,unsigned,Date,Arrived_button>(
			db,
			"SELECT subsystem,part_id,part_supplier,part_number,qty,arrival_date,part_id "
			"FROM part_info "
			"WHERE "
				"id IN (SELECT MAX(id) FROM part_info GROUP BY part_id) "
				"AND valid "
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
		qm<Subsystem_id,Part_id,Supplier,Part_number,unsigned,Date>(
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

void inner(ostream& o,Orders const& a,DB db){
	make_page(
		o,
		"Orders",
		to_order(db,a)
		+on_order(db,a)
		+arrived(db,a)
	);
}

template<typename T>
string join(string delim,vector<T> v){
	stringstream ss;
	for(auto [last,elem]:mark_last(v)){
		ss<<elem;
		if(!last) ss<<delim;
	}
	return ss.str();
}

void inner(std::ostream& o,Order_edit const& a,DB db){
	if(as_string(a.arrival_date)==""){
		make_page(
			o,
			"Order edit error",
			"Error: Order not recorded because no arrival date specified."
		);
		return;
	}
	if(a.part_checkbox.size()==0){
		make_page(
			o,
			"Order edit error",
			"No change made because no parts were selected"
		);
		return;
	}

	//edit each of the parts

	vector<string> part_members;
	#define X(A,B) part_members|=""#B;
	PART_DATA(X)
	#undef X
	
	//pull the data out
	auto q=query(
		db,
		[=](){
			stringstream ss;
			ss<<"SELECT part_id,";
			ss<<join(",",part_members);
			ss<<" ,0";
			ss<<" FROM part_info ";
			ss<<" WHERE ";
			ss<<"  valid AND";
			ss<<"  id IN (SELECT MAX(id) FROM part_info GROUP BY part_id)";
			ss<<"  AND part_id IN (";
			ss<<join(",",a.part_checkbox);
			ss<<"  )";
			return ss.str();
		}()
	);

	vector<pair<Part_id,Part_data>> found;
	for(auto row:q){
		assert(row[0]);
		Part_id part_id=parse((Part_id*)0,*row[0]);
		Part_data current;
		int i=1;
		#define X(A,B) {\
			auto x=parse((optional<A>*)0,row[i]);\
			if(x) current.B=*x;\
			i++;\
		}
		PART_DATA(X)
		#undef X
		found|=make_pair(part_id,current);
	}

	//change a couple of fields
	for(auto &item:found){
		item.second.part_state=Part_state::ordered;
		item.second.arrival_date=a.arrival_date;
	}

	//write it back
	for(auto [part_id,row]:found){
		stringstream ss;
		ss<<"INSERT INTO part_info ";
		ss<<"(";
		ss<<"part_id,edit_user,edit_date,";
		ss<<join(",",part_members);
		ss<<") VALUES ";
		ss<<"(";
		ss<<escape(part_id)<<",";
		ss<<escape(current_user())<<",";
		ss<<"now(),";
		unsigned i=0;
		#define X(A,B) {\
			ss<<escape(row.B);\
			i++;\
			if(i!=part_members.size()) ss<<",";\
		}
		PART_DATA(X)
		#undef X
		ss<<")";
		run_cmd(db,ss.str());
	}
	
	//redirect to the orders page
	make_page(o,"Order update",redirect_to(Orders{}));
}

void inner(ostream& o,Arrived const& a,DB db){
	make_page(
		o,
		"Arrived",
		as_string(a)+"Under"
	);
	//edit each of the parts

	vector<string> part_members;
	#define X(A,B) part_members|=""#B;
	PART_DATA(X)
	#undef X
	
	//pull the data out
	auto q=query(
		db,
		[=](){
			stringstream ss;
			ss<<"SELECT part_id,";
			ss<<join(",",part_members);
			ss<<" ,0";
			ss<<" FROM part_info ";
			ss<<" WHERE ";
			ss<<"  valid AND";
			ss<<"  id IN (SELECT MAX(id) FROM part_info GROUP BY part_id)";
			ss<<"  AND part_id="+escape(a.part);
			return ss.str();
		}()
	);

	vector<pair<Part_id,Part_data>> found;
	for(auto row:q){
		assert(row[0]);
		Part_id part_id=parse((Part_id*)0,*row[0]);
		Part_data current;
		int i=1;
		#define X(A,B) {\
			auto x=parse((optional<A>*)0,row[i]);\
			if(x) current.B=*x;\
			i++;\
		}
		PART_DATA(X)
		#undef X
		found|=make_pair(part_id,current);
	}

	//change state here; date will be later
	for(auto &item:found){
		item.second.part_state=Part_state::arrived;
	}

	//write it back
	for(auto [part_id,row]:found){
		stringstream ss;
		ss<<"INSERT INTO part_info ";
		ss<<"(";
		ss<<"part_id,edit_user,edit_date,";
		ss<<join(",",part_members);
		ss<<") VALUES ";
		ss<<"(";
		ss<<escape(part_id)<<",";
		ss<<escape(current_user())<<",";
		ss<<"now(),";
		unsigned i=0;
		#define X(A,B) {\
			if(string(""#B)=="arrival_date"){\
				ss<<"now()";\
			}else{\
				ss<<escape(row.B);\
			}\
			i++;\
			if(i!=part_members.size()) ss<<",";\
		}
		PART_DATA(X)
		#undef X
		ss<<")";
		run_cmd(db,ss.str());
	}
	
	//redirect to the orders page
	make_page(o,"Order update",redirect_to(Orders{}));
}
