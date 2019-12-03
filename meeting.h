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

#endif
