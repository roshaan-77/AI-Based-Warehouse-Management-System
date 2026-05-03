#include "admin.h"
#include "warehouse.h"
#include "user.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static int admin_login(void) {
    char username[ITEM_NAME_LEN];
    char password[PASSWORD_LEN];

    printf("\n===== ADMIN LOGIN =====\n");
    printf("Enter Admin Username: ");
    scanf("%s", username);

    printf("Enter Admin Password: ");
    scanf("%s", password);

    if (!authenticate_admin(username, password)) {
        printf("\nAuthentication Failed. Access Denied.\n");
        sleep(2);
        return 0;
    }

    printf("\nWelcome, %s\n", username);
    sleep(1);
    return 1;
}

static void add_supplier_account(void) {
    char name[ITEM_NAME_LEN];
    char password[PASSWORD_LEN];

    printf("Enter new Supplier name: ");
    scanf("%s", name);

    if (is_name_exists(name)) {
        printf("User already exists.\n");
        sleep(2);
        return;
    }

    printf("Enter password for Supplier '%s': ", name);
    scanf("%s", password);

    if (add_user_to_list(name, password, 1, (pthread_t)0)) {
        printf("Supplier account created successfully.\n");
    } else {
        printf("Failed to create supplier account.\n");
    }

    sleep(2);
}

static void add_retailer_account(void) {
    char name[ITEM_NAME_LEN];
    char password[PASSWORD_LEN];

    printf("Enter new Retailer name: ");
    scanf("%s", name);

    if (is_name_exists(name)) {
        printf("User already exists.\n");
        sleep(2);
        return;
    }

    printf("Enter password for Retailer '%s': ", name);
    scanf("%s", password);

    if (add_user_to_list(name, password, 0, (pthread_t)0)) {
        printf("Retailer account created successfully.\n");
    } else {
        printf("Failed to create retailer account.\n");
    }

    sleep(2);
}

static void list_registered_users(void) {
    pthread_mutex_lock(&users_mutex);

    printf("\n===== REGISTERED USERS =====\n");
    for (int i = 0; i < user_count; i++) {
        printf("%d. %s | Role: %s | Priority: %d | Running: %s\n",
               i + 1,
               active_users[i].name,
               active_users[i].is_supplier ? "Supplier" : "Retailer",
               active_users[i].priority,
               active_users[i].running ? "Yes" : "No");
    }

    if (user_count == 0) {
        printf("No supplier/retailer accounts created yet.\n");
    }

    pthread_mutex_unlock(&users_mutex);

    printf("\nPress any key to continue\n");
    clear_input_buffer();
    my_getch();
}

static void configure_operation_limit(void) {
    int limit;

    printf("Enter total operation limit (0 for unlimited): ");
    scanf("%d", &limit);
    if (limit < 0) limit = 0;

    pthread_mutex_lock(&warehouse->mutex);
    warehouse->operation_limit = limit;
    warehouse->simulation_running = 1;
    warehouse->shutdown_requested = 0;
    snprintf(warehouse->last_event, sizeof(warehouse->last_event),
             "Operation limit set to %d", limit);
    pthread_mutex_unlock(&warehouse->mutex);

    printf("Operation limit updated to %d.\n", limit);
    sleep(2);
}

void admin_panel(void) {
    if (!admin_login()) return;

    int choice;
    do {
        system("clear");
        printf("\n===== ADMIN PANEL =====\n");
        printf("1. Create Supplier Account\n");
        printf("2. Create Retailer Account\n");
        printf("3. Start Supplier Thread\n");
        printf("4. Start Retailer Thread\n");
        printf("5. View System Log\n");
        printf("6. View Registered Users\n");
        printf("7. Set Demo Operation Limit\n");
        printf("8. Stop Simulation\n");
        printf("9. Reset Runtime Counters\n");
        printf("10. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                add_supplier_account();
                break;

            case 2:
                add_retailer_account();
                break;

            case 3: {
                char supplier_name[ITEM_NAME_LEN];
                printf("Enter Supplier name to start: ");
                scanf("%s", supplier_name);

                if (!is_name_exists(supplier_name) || get_user_role(supplier_name) != 1) {
                    printf("Supplier account does not exist.\n");
                    sleep(2);
                    break;
                }

                start_supplier_thread(supplier_name);
                sleep(2);
                break;
            }

            case 4: {
                char retailer_name[ITEM_NAME_LEN];
                printf("Enter Retailer name to start: ");
                scanf("%s", retailer_name);

                if (!is_name_exists(retailer_name) || get_user_role(retailer_name) != 0) {
                    printf("Retailer account does not exist.\n");
                    sleep(2);
                    break;
                }

                start_retailer_thread(retailer_name);
                sleep(2);
                break;
            }

            case 5:
                print_log(SYSTEM_LOG);
                printf("\nPress any key to continue\n");
                clear_input_buffer();
                my_getch();
                break;

            case 6:
                list_registered_users();
                break;

            case 7:
                configure_operation_limit();
                break;

            case 8:
                request_simulation_stop();
                printf("Simulation stopped.\n");
                sleep(2);
                break;

            case 9:
                reset_warehouse_runtime();
                printf("Runtime counters reset.\n");
                sleep(2);
                break;

            case 10:
                return;

            default:
                printf("Invalid choice. Please try again.\n");
                sleep(1);
        }
    } while (1);
}
