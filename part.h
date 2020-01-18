#ifndef PART_H
#define PART_H

#include "queries.h"

void inner(std::ostream&,Part_new const&,DB);
void inner(std::ostream&,Part_editor const&,DB);
void inner(std::ostream&,Part_edit const&,DB);
void inner(std::ostream&,Part_duplicate const&,DB);
void inner(std::ostream&,Batch_entry const&,DB);
void inner(std::ostream&,Batch_entry_backend const&,DB);

Part_number_local next_part_number(DB,Subsystem_id);
std::optional<Part_data> part_data(DB,Part_id);
void insert_part_data(DB,Part_id,Part_data const&);
std::string after_done();
std::string redirect_to(URL const&);
std::string fields_by_state();
bool should_show(Part_state,std::string const& name);

template<typename T>
std::pair<std::string,std::string> part_entry(DB db,std::optional<Subsystem_id> subsystem_id,T t){
	if( (as_string(t)=="" || as_string(t)=="NULL") && subsystem_id ){
		//give it the next free part number for whatever assembly it's in.
		return std::make_pair("part_number",escape(next_part_number(db,*subsystem_id)));
	}
	return std::make_pair("part_number",escape(t));
}

#endif
