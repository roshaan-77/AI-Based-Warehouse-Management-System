#ifndef RETAILER_H
#define RETAILER_H

#include "user.h"
#include "warehouse.h"
#include <pthread.h>

void retailer_panel(char *retailer_name);
void *retailer_function(void *arg);

#endif
