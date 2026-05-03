#include "retailer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

void *retailer_function(void *arg) {
    char *retailer_name = (char *)arg;
    Item consumed_item;

    set_user_running(pthread_self(), 1);

    while (!warehouse->shutdown_requested && warehouse->simulation_running) {
        int my_priority = 0;

        pthread_mutex_lock(&users_mutex);
        for (int i = 0; i < user_count; i++) {
            if (strcmp(active_users[i].name, retailer_name) == 0) {
                my_priority = active_users[i].priority;
                break;
            }
        }
        pthread_mutex_unlock(&users_mutex);

        sleep(calculate_sleep_time(my_priority) + warehouse->retailer_delay);

        if (should_operate(my_priority)) {
            if (sem_trywait(&warehouse->full) == -1) {
                pthread_mutex_lock(&warehouse->mutex);
                warehouse->retailer_waiting++;
                warehouse->retailer_block_cycles++;
                snprintf(warehouse->last_event, sizeof(warehouse->last_event),
                         "Retailer %s is waiting because buffer is empty", retailer_name);
                pthread_mutex_unlock(&warehouse->mutex);

                sem_wait(&warehouse->full);

                pthread_mutex_lock(&warehouse->mutex);
                if (warehouse->retailer_waiting > 0) warehouse->retailer_waiting--;
                pthread_mutex_unlock(&warehouse->mutex);
            } else {
                pthread_mutex_lock(&warehouse->mutex);
                warehouse->retailer_block_cycles = 0;
                pthread_mutex_unlock(&warehouse->mutex);
            }

            pthread_mutex_lock(&warehouse->mutex);

            if (!warehouse->simulation_running || warehouse->buffer_count <= 0) {
                pthread_mutex_unlock(&warehouse->mutex);
                break;
            }

            consumed_item = warehouse->buffer[warehouse->out];
            warehouse->out = (warehouse->out + 1) % BUFFER_SIZE;
            warehouse->buffer_count--;
            warehouse->consumed_count++;
            warehouse->total_operations++;
            snprintf(warehouse->last_event, sizeof(warehouse->last_event),
                     "Retailer %s consumed item %d, buffer %d/%d",
                     retailer_name, consumed_item.id,
                     warehouse->buffer_count, BUFFER_SIZE);
            int reached_limit = (warehouse->operation_limit > 0 &&
                                 warehouse->total_operations >= warehouse->operation_limit);

            pthread_mutex_unlock(&warehouse->mutex);
            sem_post(&warehouse->empty);

            Item log_item;
            strcpy(log_item.name, retailer_name);
            log_item.id = consumed_item.id;
            log_item.quantity = consumed_item.quantity;
            log_item.priority = my_priority;

            log_action(SYSTEM_LOG, "Retailed", log_item);

            if (reached_limit) {
                request_simulation_stop();
            }
        }
    }

    remove_user_from_list(pthread_self());
    free(arg);
    return NULL;
}

void retailer_panel(char *retailer_name) {
    if (!is_name_exists(retailer_name)) {
        printf("Error: Retailer '%s' doesn't exist.\n", retailer_name);
        sleep(2);
        return;
    }

    char password[PASSWORD_LEN];
    printf("Enter Password for Retailer '%s': ", retailer_name);
    scanf("%s", password);

    if (!authenticate_user(retailer_name, password, 0)) {
        printf("\nAuthentication Failed. Access Denied.\n");
        sleep(2);
        return;
    }

    int choice;
    do {
        system("clear");
        printf("\n===== RETAILER PANEL =====\n");
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
