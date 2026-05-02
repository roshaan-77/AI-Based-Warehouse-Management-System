#ifndef SUPPLIER_H
#define SUPPLIER_H

#include "user.h"
#include "warehouse.h"
#include <pthread.h>

void supplier_panel(char *supplier_name);
void *supplier_function(void *arg);

#endif
