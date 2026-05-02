#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>

#include "admin.h"
#include "retailer.h"
#include "supplier.h"
#include "login.h"
#include "warehouse.h"
#include "user.h"
#include "ai.h"

pthread_t ai_thread;

void handle_sigint(int sig) {
    printf("\n[!] Caught signal %d, cleaning up...\n", sig);
    cleanup_warehouse();
    exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);
    srand(time(NULL));

    initialize_warehouse();
    initialize_user_system();

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