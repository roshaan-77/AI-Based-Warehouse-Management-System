#ifndef ITEM_H
#define ITEM_H

#define ITEM_NAME_LEN 32
#define MAX_ITEMS 10
#define BUFFER_SIZE 5
#define MAX_PRIORITY 10

typedef struct {
    char name[ITEM_NAME_LEN];
    int id;
    int quantity;
    int priority;
} Item;

#endif
