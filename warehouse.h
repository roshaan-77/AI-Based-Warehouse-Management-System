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
    int total_operations;
    int operation_limit;
    int simulation_running;
    int shutdown_requested;

    // Waiting thread counters
    int supplier_waiting;
    int retailer_waiting;
    int supplier_block_cycles;
    int retailer_block_cycles;

    // AI-controlled delays
    int producer_delay;
    int retailer_delay;

    // AI status
    int starvation_flag;
    int last_ai_state;
    int last_ai_action;
    int last_ai_reward;
    float last_ai_q_value;
    float last_ai_best_q;
    char last_ai_reason[128];
    char last_event[160];

    // Q-learning table: 6 states x 3 actions
    float q_table[6][3];

} Warehouse;

extern Warehouse *warehouse;

void initialize_warehouse();
void print_log(const char *filename);
void start_supplier_thread(char *supplier_name);
void start_retailer_thread(char *retailer_name);
void request_simulation_stop(void);
void reset_warehouse_runtime(void);
void cleanup_warehouse();

#endif
