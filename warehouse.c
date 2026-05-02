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

    warehouse->supplier_waiting = 0;
    warehouse->retailer_waiting = 0;

    warehouse->producer_delay = 2;
    warehouse->retailer_delay = 2;

    warehouse->starvation_flag = 0;
    warehouse->last_ai_state = -1;
    warehouse->last_ai_action = -1;
    warehouse->last_ai_reward = 0;

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

void cleanup_warehouse() {
    if (warehouse != NULL) {
        munmap(warehouse, sizeof(Warehouse));
        shm_unlink(SHM_NAME);
    }
}