#ifndef PART_H
#define PART_H

#include "queries.h"

void inner(std::ostream&,Part_new const&,DB);
void inner(std::ostream&,Part_editor const&,DB);
void inner(std::ostream&,Part_edit const&,DB);

#endif
