#include "user.h"
#include "warehouse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

ActiveUser active_users[MAX_USERS];
int user_count = 0;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

static int find_user_index(const char *name) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(active_users[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void trim_text(char *text) {
    size_t len;

    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') {
        memmove(text, text + 1, strlen(text));
    }

    len = strlen(text);
    while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t' ||
                       text[len - 1] == '\r' || text[len - 1] == '\n')) {
        text[len - 1] = '\0';
        len--;
    }
}

void ensure_admin_file(void) {
    FILE *fp = fopen(ADMINS_FILE, "r");
    if (fp != NULL) {
        fclose(fp);
        return;
    }

    fp = fopen(ADMINS_FILE, "w");
    if (fp == NULL) {
        perror("Unable to create admins file");
        return;
    }

    fprintf(fp, "Roshaan,0724\n");
    fprintf(fp, "Arti,0530\n");
    fprintf(fp, "Abdullah,0576\n");
    fclose(fp);
}

void load_users_from_file(void) {
    FILE *fp = fopen(USERS_FILE, "r");
    if (fp == NULL) {
        return; // first run, no users yet
    }

    user_count = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (user_count >= MAX_USERS) break;

        char *name = strtok(line, ",\r\n");
        char *password = strtok(NULL, ",\r\n");
        char *role = strtok(NULL, ",\r\n");
        char *priority = strtok(NULL, ",\r\n");

        if (name && password && role && priority) {
            strncpy(active_users[user_count].name, name, ITEM_NAME_LEN - 1);
            active_users[user_count].name[ITEM_NAME_LEN - 1] = '\0';

            strncpy(active_users[user_count].password, password, PASSWORD_LEN - 1);
            active_users[user_count].password[PASSWORD_LEN - 1] = '\0';

            active_users[user_count].is_supplier = atoi(role);
            active_users[user_count].priority = atoi(priority);

            memset(&active_users[user_count].tid, 0, sizeof(pthread_t));
            active_users[user_count].running = 0;

            user_count++;
        }
    }

    fclose(fp);
}

void save_users_to_file(void) {
    FILE *fp = fopen(USERS_FILE, "w");
    if (fp == NULL) {
        perror("Unable to save users file");
        return;
    }

    for (int i = 0; i < user_count; i++) {
        fprintf(fp, "%s,%s,%d,%d\n",
                active_users[i].name,
                active_users[i].password,
                active_users[i].is_supplier,
                active_users[i].priority);
    }

    fclose(fp);
}

void initialize_user_system(void) {
    ensure_admin_file();
    load_users_from_file();
}

int authenticate_admin(const char *username, const char *password) {
    if ((strcmp(username, "Roshaan") == 0 && strcmp(password, "0724") == 0) ||
        (strcmp(username, "Arti") == 0 && strcmp(password, "0530") == 0) ||
        (strcmp(username, "Abdullah") == 0 && strcmp(password, "0576") == 0)) {
        return 1;
    }

    FILE *fp = fopen(ADMINS_FILE, "r");
    if (fp == NULL) {
        perror("Unable to open admins file");
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *u = strtok(line, ",\r\n");
        char *p = strtok(NULL, ",\r\n");

        if (u && p) {
            trim_text(u);
            trim_text(p);
            if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
                fclose(fp);
                return 1;
            }
        }
    }

    fclose(fp);
    return 0;
}

int authenticate_user(const char *name, const char *password, int is_supplier) {
    pthread_mutex_lock(&users_mutex);

    int idx = find_user_index(name);
    if (idx == -1) {
        pthread_mutex_unlock(&users_mutex);
        return 0;
    }

    int ok = (strcmp(active_users[idx].password, password) == 0 &&
              active_users[idx].is_supplier == is_supplier);

    pthread_mutex_unlock(&users_mutex);
    return ok;
}

int add_user_to_list(const char *name, const char *password, int is_supplier, pthread_t tid) {
    pthread_mutex_lock(&users_mutex);

    int idx = find_user_index(name);
    if (idx != -1) {
        pthread_mutex_unlock(&users_mutex);
        return 0; // user already exists
    }

    if (user_count >= MAX_USERS) {
        pthread_mutex_unlock(&users_mutex);
        return 0;
    }

    strncpy(active_users[user_count].name, name, ITEM_NAME_LEN - 1);
    active_users[user_count].name[ITEM_NAME_LEN - 1] = '\0';

    strncpy(active_users[user_count].password, password, PASSWORD_LEN - 1);
    active_users[user_count].password[PASSWORD_LEN - 1] = '\0';

    active_users[user_count].is_supplier = is_supplier;
    active_users[user_count].priority = 1;
    active_users[user_count].tid = tid;
    active_users[user_count].running = 0;

    user_count++;

    pthread_mutex_unlock(&users_mutex);

    save_users_to_file();
    return 1;
}

int is_name_exists(const char *name) {
    pthread_mutex_lock(&users_mutex);
    int exists = (find_user_index(name) != -1);
    pthread_mutex_unlock(&users_mutex);
    return exists;
}

int get_user_role(const char *name) {
    pthread_mutex_lock(&users_mutex);
    int idx = find_user_index(name);
    int role = -1;
    if (idx != -1) role = active_users[idx].is_supplier;
    pthread_mutex_unlock(&users_mutex);
    return role;
}

int is_user_running(const char *name) {
    pthread_mutex_lock(&users_mutex);
    int idx = find_user_index(name);
    int running = 0;
    if (idx != -1) running = active_users[idx].running;
    pthread_mutex_unlock(&users_mutex);
    return running;
}

void attach_thread_to_user(const char *name, pthread_t tid) {
    pthread_mutex_lock(&users_mutex);
    int idx = find_user_index(name);
    if (idx != -1) {
        active_users[idx].tid = tid;
        active_users[idx].running = 1;
    }
    pthread_mutex_unlock(&users_mutex);
}

void set_user_running(pthread_t tid, int running) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < user_count; i++) {
        if (active_users[i].running && pthread_equal(active_users[i].tid, tid)) {
            active_users[i].running = running;
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);
}

void remove_user_from_list(pthread_t tid) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < user_count; i++) {
        if (active_users[i].running && pthread_equal(active_users[i].tid, tid)) {
            active_users[i].running = 0;
            memset(&active_users[i].tid, 0, sizeof(pthread_t));
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);
}

int calculate_sleep_time(int priority) {
    if (priority <= 1) return 3;
    if (priority == 2) return 2;
    return 1;
}

int should_operate(int priority) {
    (void)priority;
    return 1;
}

void log_action(const char *filename_ignored, const char *action, Item item) {
    (void)filename_ignored;

    FILE *fp = fopen(SYSTEM_LOG, "a");
    if (fp == NULL) {
        perror("Unable to open system log");
        return;
    }

    time_t now = time(NULL);
    char *ts = ctime(&now);
    if (ts != NULL) {
        ts[strcspn(ts, "\n")] = '\0';
    }

    int buffer_count = 0;
    int produced = 0;
    int consumed = 0;
    int producer_delay = 0;
    int retailer_delay = 0;

    if (warehouse != NULL) {
        pthread_mutex_lock(&warehouse->mutex);
        buffer_count = warehouse->buffer_count;
        produced = warehouse->produced_count;
        consumed = warehouse->consumed_count;
        producer_delay = warehouse->producer_delay;
        retailer_delay = warehouse->retailer_delay;
        pthread_mutex_unlock(&warehouse->mutex);
    }

    fprintf(fp,
            "%s | %s | Name: %s | ID: %d | Qty: %d | Priority: %d | Buffer: %d/%d | Produced: %d | Consumed: %d | Delays P/R: %d/%d\n",
            ts ? ts : "UnknownTime",
            action,
            item.name,
            item.id,
            item.quantity,
            item.priority,
            buffer_count,
            BUFFER_SIZE,
            produced,
            consumed,
            producer_delay,
            retailer_delay);

    fclose(fp);
}
