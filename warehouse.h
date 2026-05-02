#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include "item.h"
#include "supplier.h"
#include "retailer.h"
#include "user.h"
#include <semaphore.h>
#include <pthread.h>
#include <string.h>

#define SHM_NAME "/warehouse_shm"

typedef struct {
    Item buffer[BUFFER_SIZE];
    int in, out;

    pthread_mutex_t mutex;
    sem_t full, empty;

    // Current number of items in buffer
    int buffer_count;

    // Statistics
    int produced_count;
    int consumed_count;

    // Waiting thread counters
    int supplier_waiting;
    int retailer_waiting;

    // AI-controlled delays
    int producer_delay;
    int retailer_delay;

    // AI status
    int starvation_flag;
    int last_ai_state;
    int last_ai_action;
    int last_ai_reward;

    // Q-learning table: 6 states x 3 actions
    float q_table[6][3];

} Warehouse;

extern Warehouse *warehouse;

void initialize_warehouse();
void print_log(const char *filename);
void start_supplier_thread(char *supplier_name);
void start_retailer_thread(char *retailer_name);
void cleanup_warehouse();

#endif