#ifndef ORDER_H
#define ORDER_H

#include "queries.h"

void inner(std::ostream&,Orders const&,DB);
void inner(std::ostream&,Order_edit const&,DB);
void inner(std::ostream&,Arrived const&,DB);

#endif
