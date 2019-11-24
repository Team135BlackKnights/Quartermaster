#ifndef SUBSYSTEM_H
#define SUBSYSTEM_H

#include "queries.h"

void inner(std::ostream& o,Subsystem_new const&,DB);
void inner(std::ostream& o,Subsystem_edit const&,DB);
void inner(std::ostream& o,Subsystem_editor const&,DB);

template<typename T>
std::string redirect_to(T const& t){
	std::stringstream ss;
	//ss<<"<meta http-equiv = \"refresh\" content = \"2; url = ";
	ss<<"<meta http-equiv = \"refresh\" content = \"0; url = ";
	ss<<"?"<<to_query(t);
	ss<<"\" />";
	ss<<"You should have been automatically redirected to:"<<t<<"\n";
	return ss.str();
}

Id new_item(DB,Table_name const&);
std::string current_user();

#endif
