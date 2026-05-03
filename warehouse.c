#include "warehouse.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

Warehouse *warehouse;

void initialize_warehouse() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(1);
    }

    if (ftruncate(shm_fd, sizeof(Warehouse)) == -1) {
        perror("ftruncate failed");
        exit(1);
    }

    warehouse = mmap(0, sizeof(Warehouse), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (warehouse == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    warehouse->in = 0;
    warehouse->out = 0;

    warehouse->buffer_count = 0;
    warehouse->produced_count = 0;
    warehouse->consumed_count = 0;
    warehouse->total_operations = 0;
    warehouse->operation_limit = 0;
    warehouse->simulation_running = 1;
    warehouse->shutdown_requested = 0;

    warehouse->supplier_waiting = 0;
    warehouse->retailer_waiting = 0;
    warehouse->supplier_block_cycles = 0;
    warehouse->retailer_block_cycles = 0;

    warehouse->producer_delay = 2;
    warehouse->retailer_delay = 2;

    warehouse->starvation_flag = 0;
    warehouse->last_ai_state = -1;
    warehouse->last_ai_action = -1;
    warehouse->last_ai_reward = 0;
    warehouse->last_ai_q_value = 0.0f;
    warehouse->last_ai_best_q = 0.0f;
    snprintf(warehouse->last_ai_reason, sizeof(warehouse->last_ai_reason),
             "AI is waiting for warehouse activity");
    snprintf(warehouse->last_event, sizeof(warehouse->last_event),
             "Warehouse initialized");

    memset(warehouse->q_table, 0, sizeof(warehouse->q_table));

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&warehouse->mutex, &mattr);

    sem_init(&warehouse->full, 1, 0);
    sem_init(&warehouse->empty, 1, BUFFER_SIZE);
}

void print_log(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Unable to open file");
        return;
    }

    char ch;
    while ((ch = fgetc(fp)) != EOF) {
        putchar(ch);
    }
    fclose(fp);
}

void start_supplier_thread(char *supplier_name) {
    if (!is_name_exists(supplier_name) || get_user_role(supplier_name) != 1) {
        printf("Supplier account does not exist.\n");
        return;
    }

    if (is_user_running(supplier_name)) {
        printf("[Supplier] Thread already running for: %s\n", supplier_name);
        return;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, supplier_function, strdup(supplier_name));
    attach_thread_to_user(supplier_name, tid);

    printf("[Supplier] Started thread for supplier: %s\n", supplier_name);
}

void start_retailer_thread(char *retailer_name) {
    if (!is_name_exists(retailer_name) || get_user_role(retailer_name) != 0) {
        printf("Retailer account does not exist.\n");
        return;
    }

    if (is_user_running(retailer_name)) {
        printf("[Retailer] Thread already running for: %s\n", retailer_name);
        return;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, retailer_function, strdup(retailer_name));
    attach_thread_to_user(retailer_name, tid);

    printf("[Retailer] Started thread for retailer: %s\n", retailer_name);
}

void request_simulation_stop(void) {
    if (warehouse == NULL) return;

    pthread_mutex_lock(&warehouse->mutex);
    warehouse->simulation_running = 0;
    snprintf(warehouse->last_event, sizeof(warehouse->last_event),
             "Simulation stop requested");
    pthread_mutex_unlock(&warehouse->mutex);

    for (int i = 0; i < BUFFER_SIZE + 8; i++) {
        sem_post(&warehouse->empty);
        sem_post(&warehouse->full);
    }
}

void reset_warehouse_runtime(void) {
    if (warehouse == NULL) return;

    pthread_mutex_lock(&warehouse->mutex);
    warehouse->in = 0;
    warehouse->out = 0;
    warehouse->buffer_count = 0;
    warehouse->produced_count = 0;
    warehouse->consumed_count = 0;
    warehouse->total_operations = 0;
    warehouse->supplier_waiting = 0;
    warehouse->retailer_waiting = 0;
    warehouse->supplier_block_cycles = 0;
    warehouse->retailer_block_cycles = 0;
    warehouse->producer_delay = 2;
    warehouse->retailer_delay = 2;
    warehouse->starvation_flag = 0;
    warehouse->last_ai_state = -1;
    warehouse->last_ai_action = -1;
    warehouse->last_ai_reward = 0;
    warehouse->last_ai_q_value = 0.0f;
    warehouse->last_ai_best_q = 0.0f;
    warehouse->simulation_running = 1;
    warehouse->shutdown_requested = 0;
    memset(warehouse->q_table, 0, sizeof(warehouse->q_table));
    snprintf(warehouse->last_ai_reason, sizeof(warehouse->last_ai_reason),
             "AI reset and ready");
    snprintf(warehouse->last_event, sizeof(warehouse->last_event),
             "Warehouse runtime reset");
    pthread_mutex_unlock(&warehouse->mutex);

    sem_destroy(&warehouse->full);
    sem_destroy(&warehouse->empty);
    sem_init(&warehouse->full, 1, 0);
    sem_init(&warehouse->empty, 1, BUFFER_SIZE);
}

void cleanup_warehouse() {
    if (warehouse != NULL) {
        pthread_mutex_lock(&warehouse->mutex);
        warehouse->shutdown_requested = 1;
        pthread_mutex_unlock(&warehouse->mutex);
        request_simulation_stop();
        munmap(warehouse, sizeof(Warehouse));
        shm_unlink(SHM_NAME);
    }
}
