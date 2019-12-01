#ifndef PART_H
#define PART_H

#include "queries.h"

void inner(std::ostream&,Part_new const&,DB);
void inner(std::ostream&,Part_editor const&,DB);
void inner(std::ostream&,Part_edit const&,DB);
void inner(std::ostream&,Part_duplicate const&,DB);
Part_number_local next_part_number(DB,Subsystem_id);

template<typename T>
std::pair<std::string,std::string> part_entry(DB db,std::optional<Subsystem_id> subsystem_id,T t){
	if( (as_string(t)=="" || as_string(t)=="NULL") && subsystem_id ){
		//give it the next free part number for whatever assembly it's in.
		return std::make_pair("part_number",escape(next_part_number(db,*subsystem_id)));
	}
	return std::make_pair("part_number",escape(t));
}

#endif
