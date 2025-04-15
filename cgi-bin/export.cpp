#include "export.h"
#include<string.h>

using namespace std;

string csv_escape(string s){
	if(strstr(s.c_str(),",")){
		stringstream ss;
		ss<<"\"";
		for(auto c:s){
			if(c=='"'){
				ss<<"\\"<<c;
			}else if(c=='\\'){
				ss<<"\\\\";
			}else{
				ss<<c;
			}
		}
		ss<<"\"";
		return ss.str();
	}
	return s;
}

string as_csv(DB db,string table_name){
	auto columns=mapf([](auto x){ return *x; },firsts(query(db,"DESCRIBE "+table_name)));
	auto q=query(db,"SELECT * FROM "+table_name);
	stringstream ss;
	for(auto col:columns){
		ss<<col<<",";
	}
	ss<<"\n";
	for(auto row:q){
		for(auto elem:row){
			if(elem){
				ss<<csv_escape(*elem)<<",";
			}else{
				ss<<""; //empty
			}
		}
		ss<<"\n";
	}
	return ss.str();
}

void inner(ostream& o,CSV_export const& a,DB db){
	o<<"Content-type: text/csv\n";
	o<<"Expires: 0\n\n";

	switch(a.export_item){
		case Export_item::SUBSYSTEM:
			o<<as_csv(db,"subsystem");
			return;
		case Export_item::SUBSYSTEM_INFO:
			o<<as_csv(db,"subsystem_info");
			return;
		case Export_item::PART:
			o<<as_csv(db,"part");
			return;
		case Export_item::PART_INFO:
			o<<as_csv(db,"part_info");
			return;
		case Export_item::MEETING:
			o<<as_csv(db,"meeting");
			return;
		case Export_item::MEETING_INFO:
			o<<as_csv(db,"meeting_info");
			return;
		default:
			assert(0);
	}
}

string link(Export_item a){
	CSV_export e;
	e.export_item=a;
	return link(e,as_string(a));
}

string export_links(){
	stringstream ss;
	ss<<h2("CSV export");
	#define X(A) ss<<link(Export_item::A)<<"<br>";
	EXPORT_ITEMS(X)
	#undef X
	return ss.str();
}
