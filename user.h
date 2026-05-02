#ifndef USER_H
#define USER_H

#include <pthread.h>
#include "item.h"

#define MAX_USERS 100
#define PASSWORD_LEN 64

#define USERS_FILE "users.txt"
#define ADMINS_FILE "admins.txt"
#define SYSTEM_LOG "system_log.txt"

typedef struct {
    char name[ITEM_NAME_LEN];
    char password[PASSWORD_LEN];
    int is_supplier;   // 1 = supplier, 0 = retailer
    int priority;      // default 1
    pthread_t tid;     // runtime only
    int running;       // runtime only
} ActiveUser;

extern ActiveUser active_users[MAX_USERS];
extern int user_count;
extern pthread_mutex_t users_mutex;

void initialize_user_system(void);
void ensure_admin_file(void);
void load_users_from_file(void);
void save_users_to_file(void);

int authenticate_admin(const char *username, const char *password);
int authenticate_user(const char *name, const char *password, int is_supplier);
int add_user_to_list(const char *name, const char *password, int is_supplier, pthread_t tid);

int is_name_exists(const char *name);
int get_user_role(const char *name);
int is_user_running(const char *name);
void attach_thread_to_user(const char *name, pthread_t tid);

void set_user_running(pthread_t tid, int running);
void remove_user_from_list(pthread_t tid);

int calculate_sleep_time(int priority);
int should_operate(int priority);

void log_action(const char *filename_ignored, const char *action, Item item);

void clear_input_buffer(void);
int my_getch(void);

#endif