#include "supplier.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

void *supplier_function(void *arg) {
    char *supplier_name = (char *)arg;

    set_user_running(pthread_self(), 1);

    while (!warehouse->shutdown_requested && warehouse->simulation_running) {
        int my_priority = 0;

        pthread_mutex_lock(&users_mutex);
        for (int i = 0; i < user_count; i++) {
            if (strcmp(active_users[i].name, supplier_name) == 0) {
                my_priority = active_users[i].priority;
                break;
            }
        }
        pthread_mutex_unlock(&users_mutex);

        sleep(calculate_sleep_time(my_priority) + warehouse->producer_delay);

        if (should_operate(my_priority)) {
            Item item;
            strcpy(item.name, supplier_name);
            item.id = rand() % 1000 + 1;
            item.quantity = rand() % 10 + 1;
            item.priority = my_priority;

            if (sem_trywait(&warehouse->empty) == -1) {
                pthread_mutex_lock(&warehouse->mutex);
                warehouse->supplier_waiting++;
                warehouse->supplier_block_cycles++;
                snprintf(warehouse->last_event, sizeof(warehouse->last_event),
                         "Supplier %s is waiting because buffer is full", supplier_name);
                pthread_mutex_unlock(&warehouse->mutex);

                sem_wait(&warehouse->empty);

                pthread_mutex_lock(&warehouse->mutex);
                if (warehouse->supplier_waiting > 0) warehouse->supplier_waiting--;
                pthread_mutex_unlock(&warehouse->mutex);
            } else {
                pthread_mutex_lock(&warehouse->mutex);
                warehouse->supplier_block_cycles = 0;
                pthread_mutex_unlock(&warehouse->mutex);
            }

            pthread_mutex_lock(&warehouse->mutex);

            if (!warehouse->simulation_running || warehouse->buffer_count >= BUFFER_SIZE) {
                pthread_mutex_unlock(&warehouse->mutex);
                break;
            }

            warehouse->buffer[warehouse->in] = item;
            warehouse->in = (warehouse->in + 1) % BUFFER_SIZE;
            warehouse->buffer_count++;
            warehouse->produced_count++;
            warehouse->total_operations++;
            snprintf(warehouse->last_event, sizeof(warehouse->last_event),
                     "Supplier %s produced item %d, buffer %d/%d",
                     supplier_name, item.id, warehouse->buffer_count, BUFFER_SIZE);
            int reached_limit = (warehouse->operation_limit > 0 &&
                                 warehouse->total_operations >= warehouse->operation_limit);

            pthread_mutex_unlock(&warehouse->mutex);
            sem_post(&warehouse->full);

            log_action(SYSTEM_LOG, "Supplied", item);

            if (reached_limit) {
                request_simulation_stop();
            }
        }
    }

    remove_user_from_list(pthread_self());
    free(arg);
    return NULL;
}

void supplier_panel(char *supplier_name) {
    if (!is_name_exists(supplier_name)) {
        printf("Error: Supplier '%s' doesn't exist.\n", supplier_name);
        sleep(2);
        return;
    }

    char password[PASSWORD_LEN];
    printf("Enter Password for Supplier '%s': ", supplier_name);
    scanf("%s", password);

    if (!authenticate_user(supplier_name, password, 1)) {
        printf("\nAuthentication Failed. Access Denied.\n");
        sleep(2);
        return;
    }

    int choice;
    do {
        system("clear");
        printf("\n===== SUPPLIER PANEL =====\n");
        printf("1. View System Log\n");
        printf("2. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                print_log(SYSTEM_LOG);
                printf("\nPress any key to continue\n");
                clear_input_buffer();
                my_getch();
                break;

            case 2:
                return;

            default:
                printf("Invalid choice. Please try again.\n");
                sleep(1);
        }
    } while (1);
}
