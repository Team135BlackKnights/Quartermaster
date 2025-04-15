#ifndef PROGRESS_H
#define PROGRESS_H

#include "util.h"
#include "queries.h"

void progress(DB);
void inner(std::ostream&,State_change const&,DB);
void in_state_by_date(std::ostream&,DB);

#endif
