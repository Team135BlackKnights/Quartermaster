#ifndef PARTS_H
#define PARTS_H

#include "queries.h"

std::optional<Request> parse_referer(const char*);
void inner(std::ostream&,New_user const&,DB);

#endif
