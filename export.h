#ifndef EXPORT_H
#define EXPORT_H

#include "queries.h"

void inner(std::ostream&,CSV_export const&,DB);
std::string export_links();

#endif
