#ifndef MEETING_H
#define MEETING_H

#include "queries.h"

void inner(std::ostream&,Meeting_new const&,DB);
void inner(std::ostream&,Meeting_editor const&,DB);
void inner(std::ostream&,Meeting_edit const&,DB);
void inner(std::ostream&,Calendar const&,DB);

std::string to_do(DB,Request const&);
std::string input_table(std::vector<Input> const&);
void make_plan(DB);
void insert(DB,Table_name const&,std::vector<std::pair<std::string,std::string>> const&);
void timeline(DB);
std::string completion_est(DB,Part_id const&);
std::string completion_est(DB,Subsystem_id const&);

#endif
