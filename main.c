#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "admin.h"
#include "retailer.h"
#include "supplier.h"
#include "login.h"
#include "warehouse.h"
#include "user.h"
#include "ai.h"
#include "ui.h"

pthread_t ai_thread;

static void create_demo_user_if_needed(const char *name, int is_supplier) {
    if (!is_name_exists(name)) {
        add_user_to_list(name, "demo123", is_supplier, (pthread_t)0);
    }
}

static void run_demo_mode(int supplier_count, int retailer_count, int operation_limit) {
    if (supplier_count < 1) supplier_count = 1;
    if (retailer_count < 1) retailer_count = 1;
    if (supplier_count > 10) supplier_count = 10;
    if (retailer_count > 10) retailer_count = 10;

    pthread_mutex_lock(&warehouse->mutex);
    warehouse->operation_limit = operation_limit;
    warehouse->simulation_running = 1;
    warehouse->shutdown_requested = 0;
    snprintf(warehouse->last_event, sizeof(warehouse->last_event),
             "Demo mode started with %d suppliers and %d retailers",
             supplier_count, retailer_count);
    pthread_mutex_unlock(&warehouse->mutex);

    for (int i = 1; i <= supplier_count; i++) {
        char name[ITEM_NAME_LEN];
        snprintf(name, sizeof(name), "Supplier%d", i);
        create_demo_user_if_needed(name, 1);
        start_supplier_thread(name);
    }

    for (int i = 1; i <= retailer_count; i++) {
        char name[ITEM_NAME_LEN];
        snprintf(name, sizeof(name), "Retailer%d", i);
        create_demo_user_if_needed(name, 0);
        start_retailer_thread(name);
    }

    printf("Demo engine is running. Open another Ubuntu terminal and run: ./dashboard\n");
    printf("Press Ctrl+C here when your demonstration is finished.\n");

    while (1) {
        sleep(1);
    }
}

void handle_sigint(int sig) {
    printf("\n[!] Caught signal %d, cleaning up...\n", sig);
    cleanup_warehouse();
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);
    srand(time(NULL));

    initialize_warehouse();
    initialize_user_system();
    pthread_create(&ai_thread, NULL, ai_scheduler_thread, NULL);
    pthread_detach(ai_thread);

    if (argc >= 2 && strcmp(argv[1], "--demo") == 0) {
        int supplier_count = argc >= 3 ? atoi(argv[2]) : 2;
        int retailer_count = argc >= 4 ? atoi(argv[3]) : 2;
        int operation_limit = argc >= 5 ? atoi(argv[4]) : 60;
        run_demo_mode(supplier_count, retailer_count, operation_limit);
    }

    if (argc < 2 || strcmp(argv[1], "--ui") == 0) {
        run_ncurses_interface();
        cleanup_warehouse();
        return 0;
    }

    if (strcmp(argv[1], "--legacy") != 0) {
        printf("Unknown option. Use ./warehouse_system, ./warehouse_system --legacy, or ./warehouse_system --demo.\n");
        cleanup_warehouse();
        return 1;
    }

    loadingBar();
    system("clear");

    int choice;
    do {
        system("clear");
        printf("=======================================\n");
        printf("  Welcome to Warehouse Management System\n");
        printf("=======================================\n");
        printf(" Ready to manage inventory and operations.\n\n");
        printf("1. Admin Panel\n");
        printf("2. Supplier Panel\n");
        printf("3. Retailer Panel\n");
        printf("4. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                admin_panel();
                break;

            case 2: {
                char supplier_name[ITEM_NAME_LEN];
                printf("Enter Supplier name: ");
                scanf("%s", supplier_name);

                if (!is_name_exists(supplier_name) || get_user_role(supplier_name) != 1) {
                    printf("Error: Supplier '%s' doesn't exist.\n", supplier_name);
                    sleep(2);
                    break;
                }

                supplier_panel(supplier_name);
                break;
            }

            case 3: {
                char retailer_name[ITEM_NAME_LEN];
                printf("Enter Retailer name: ");
                scanf("%s", retailer_name);

                if (!is_name_exists(retailer_name) || get_user_role(retailer_name) != 0) {
                    printf("Error: Retailer '%s' doesn't exist.\n", retailer_name);
                    sleep(2);
                    break;
                }

                retailer_panel(retailer_name);
                break;
            }

            case 4:
                handle_sigint(SIGINT);
                break;

            default:
                printf("Invalid choice. Please try again.\n");
                sleep(1);
        }
    } while (1);

    return 0;
}
