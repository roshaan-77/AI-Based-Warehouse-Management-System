#include "supplier.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

void *supplier_function(void *arg) {
    char *supplier_name = (char *)arg;

    set_user_running(pthread_self(), 1);

    while (1) {
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

            warehouse->supplier_waiting++;
            sem_wait(&warehouse->empty);
            warehouse->supplier_waiting--;

            pthread_mutex_lock(&warehouse->mutex);

            warehouse->buffer[warehouse->in] = item;
            warehouse->in = (warehouse->in + 1) % BUFFER_SIZE;
            warehouse->buffer_count++;
            warehouse->produced_count++;

            pthread_mutex_unlock(&warehouse->mutex);
            sem_post(&warehouse->full);

            log_action(SYSTEM_LOG, "Supplied", item);
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