#include "ui.h"

#include "ai.h"
#include "retailer.h"
#include "supplier.h"
#include "warehouse.h"
#include "user.h"

#include <ctype.h>
#include <ncurses.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PANEL_W 42

static char status_message[160] = "Login as admin to manage the warehouse.";
static int admin_logged_in = 0;
static int selected_user = 0;
static int show_help = 1;
static int selected_tab = 0;

static void set_status(const char *message) {
    snprintf(status_message, sizeof(status_message), "%s", message);
}

static void read_field(WINDOW *win, int row, int col, const char *label,
                       char *buffer, int size, int hidden) {
    echo();
    curs_set(1);
    mvwprintw(win, row, col, "%s", label);
    wclrtoeol(win);
    wrefresh(win);

    if (hidden) {
        noecho();
    }

    wgetnstr(win, buffer, size - 1);
    buffer[size - 1] = '\0';

    noecho();
    curs_set(0);
}

static void prompt_add_user(int is_supplier) {
    WINDOW *dialog = newwin(9, 54, 6, 8);
    box(dialog, 0, 0);
    keypad(dialog, TRUE);

    char name[ITEM_NAME_LEN] = "";
    char password[PASSWORD_LEN] = "";

    mvwprintw(dialog, 1, 2, "%s Account", is_supplier ? "Create Supplier" : "Create Retailer");
    read_field(dialog, 3, 2, "Name: ", name, sizeof(name), 0);
    read_field(dialog, 5, 2, "Password: ", password, sizeof(password), 1);

    if (strlen(name) == 0 || strlen(password) == 0) {
        set_status("Name/password cannot be empty.");
    } else if (is_name_exists(name)) {
        set_status("That user already exists.");
    } else if (add_user_to_list(name, password, is_supplier, (pthread_t)0)) {
        set_status(is_supplier ? "Supplier account saved." : "Retailer account saved.");
    } else {
        set_status("Unable to save account.");
    }

    delwin(dialog);
}

static void prompt_admin_login(void) {
    WINDOW *dialog = newwin(10, 54, 6, 8);
    box(dialog, 0, 0);
    keypad(dialog, TRUE);

    char username[ITEM_NAME_LEN] = "";
    char password[PASSWORD_LEN] = "";

    mvwprintw(dialog, 1, 2, "Admin Login");
    read_field(dialog, 3, 2, "Username: ", username, sizeof(username), 0);
    read_field(dialog, 5, 2, "Password: ", password, sizeof(password), 1);

    if (authenticate_admin(username, password)) {
        admin_logged_in = 1;
        set_status("Admin logged in. You can now manage suppliers and retailers.");
    } else {
        set_status("Admin login failed.");
    }

    delwin(dialog);
}

static void prompt_operation_limit(void) {
    WINDOW *dialog = newwin(7, 54, 7, 8);
    box(dialog, 0, 0);

    char input[16] = "";
    mvwprintw(dialog, 1, 2, "Set Operation Limit");
    read_field(dialog, 3, 2, "Limit (0 unlimited): ", input, sizeof(input), 0);

    int limit = atoi(input);
    if (limit < 0) limit = 0;

    pthread_mutex_lock(&warehouse->mutex);
    warehouse->operation_limit = limit;
    warehouse->simulation_running = 1;
    warehouse->shutdown_requested = 0;
    snprintf(warehouse->last_event, sizeof(warehouse->last_event),
             "Admin set operation limit to %d", limit);
    pthread_mutex_unlock(&warehouse->mutex);

    set_status("Operation limit updated.");
    delwin(dialog);
}

static int get_selected_user_index(void) {
    pthread_mutex_lock(&users_mutex);
    if (user_count <= 0) {
        pthread_mutex_unlock(&users_mutex);
        return -1;
    }
    if (selected_user < 0) selected_user = 0;
    if (selected_user >= user_count) selected_user = user_count - 1;
    int idx = selected_user;
    pthread_mutex_unlock(&users_mutex);
    return idx;
}

static int any_user_running(void) {
    int running = 0;

    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < user_count; i++) {
        if (active_users[i].running) {
            running = 1;
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);

    return running;
}

static void start_selected_user(void) {
    int idx = get_selected_user_index();
    if (idx < 0) {
        set_status("No user selected.");
        return;
    }

    char name[ITEM_NAME_LEN];
    int is_supplier;

    pthread_mutex_lock(&users_mutex);
    snprintf(name, sizeof(name), "%s", active_users[idx].name);
    is_supplier = active_users[idx].is_supplier;
    int running = active_users[idx].running;
    pthread_mutex_unlock(&users_mutex);

    if (running) {
        set_status("Selected user is already running.");
        return;
    }

    pthread_t tid;
    if (is_supplier) {
        pthread_create(&tid, NULL, supplier_function, strdup(name));
    } else {
        pthread_create(&tid, NULL, retailer_function, strdup(name));
    }
    pthread_detach(tid);
    attach_thread_to_user(name, tid);

    if (is_supplier) {
        set_status("Selected supplier thread started.");
    } else {
        set_status("Selected retailer thread started.");
    }
}

static void start_users_by_role(int is_supplier) {
    int started = 0;

    pthread_mutex_lock(&users_mutex);
    int count = user_count;
    ActiveUser copy[MAX_USERS];
    for (int i = 0; i < count; i++) {
        copy[i] = active_users[i];
    }
    pthread_mutex_unlock(&users_mutex);

    for (int i = 0; i < count; i++) {
        if (copy[i].is_supplier == is_supplier && !copy[i].running) {
            pthread_t tid;
            char *name = strdup(copy[i].name);
            if (is_supplier) {
                pthread_create(&tid, NULL, supplier_function, name);
            } else {
                pthread_create(&tid, NULL, retailer_function, name);
            }
            pthread_detach(tid);
            attach_thread_to_user(copy[i].name, tid);
            started++;
        }
    }

    if (started == 0) {
        set_status(is_supplier ? "No ready suppliers to start." : "No ready retailers to start.");
    } else {
        char msg[80];
        snprintf(msg, sizeof(msg), "Started %d %s thread(s).",
                 started, is_supplier ? "supplier" : "retailer");
        set_status(msg);
    }
}

static void get_running_counts(int *suppliers, int *retailers) {
    *suppliers = 0;
    *retailers = 0;

    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < user_count; i++) {
        if (active_users[i].running) {
            if (active_users[i].is_supplier) (*suppliers)++;
            else (*retailers)++;
        }
    }
    pthread_mutex_unlock(&users_mutex);
}

static int load_recent_logs(char lines[][140], int max_lines) {
    FILE *fp = fopen(SYSTEM_LOG, "r");
    if (!fp) return 0;

    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        snprintf(lines[count % max_lines], 140, "%.139s", line);
        count++;
    }
    fclose(fp);

    int available = count < max_lines ? count : max_lines;
    if (count > max_lines) {
        char ordered[8][140];
        for (int i = 0; i < available; i++) {
            snprintf(ordered[i], sizeof(ordered[i]), "%s", lines[(count + i) % max_lines]);
        }
        for (int i = 0; i < available; i++) {
            snprintf(lines[i], 140, "%s", ordered[i]);
        }
    }
    return available;
}

static void draw_title(WINDOW *win, int row, int col, const char *title, int color_pair) {
    wattron(win, COLOR_PAIR(color_pair) | A_BOLD);
    mvwprintw(win, row, col, "%s", title);
    wattroff(win, COLOR_PAIR(color_pair) | A_BOLD);
}

static void draw_bar(WINDOW *win, int row, int col, int width,
                     int value, int max_value, int color_pair) {
    int filled = 0;
    if (max_value > 0) {
        filled = (value * width) / max_value;
    }
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;

    mvwprintw(win, row, col, "[");
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            wattron(win, COLOR_PAIR(color_pair) | A_REVERSE);
            waddch(win, ' ');
            wattroff(win, COLOR_PAIR(color_pair) | A_REVERSE);
        } else {
            waddch(win, '.');
        }
    }
    wprintw(win, "]");
}

static void draw_metric(WINDOW *win, int row, int col, const char *label,
                        int value, int color_pair) {
    wattron(win, COLOR_PAIR(color_pair) | A_BOLD);
    mvwprintw(win, row, col, "%-18s", label);
    wattroff(win, COLOR_PAIR(color_pair) | A_BOLD);
    wprintw(win, " %5d", value);
}

static void draw_buffer(WINDOW *win, int row, const Warehouse *snap) {
    draw_title(win, row, 2, "WAREHOUSE BUFFER", 1);
    mvwprintw(win, row + 1, 2, "Slots ");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        int occupied = i < snap->buffer_count;
        int color = occupied ? 2 : 3;
        wattron(win, COLOR_PAIR(color) | A_BOLD);
        wprintw(win, occupied ? "[###]" : "[   ]");
        wattroff(win, COLOR_PAIR(color) | A_BOLD);
    }
    wprintw(win, "  %d/%d", snap->buffer_count, BUFFER_SIZE);
    draw_bar(win, row + 2, 8, 30, snap->buffer_count, BUFFER_SIZE, 2);
}

static void draw_left_panel(WINDOW *win) {
    werase(win);
    box(win, 0, 0);

    draw_title(win, 1, 2, "WAREHOUSE CONTROL ROOM", 1);
    mvwprintw(win, 2, 2, "Admin-driven OS simulation");

    mvwprintw(win, 3, 2, "Admin");
    if (admin_logged_in) {
        wattron(win, COLOR_PAIR(2) | A_BOLD);
        wprintw(win, "  AUTHORIZED");
        wattroff(win, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattron(win, COLOR_PAIR(5) | A_BOLD);
        wprintw(win, "  LOCKED");
        wattroff(win, COLOR_PAIR(5) | A_BOLD);
    }

    draw_title(win, 5, 2, "ACTIONS", 4);
    mvwprintw(win, 6, 2, "[A] Admin login");
    mvwprintw(win, 7, 2, "[S] Add supplier");
    mvwprintw(win, 8, 2, "[R] Add retailer");
    mvwprintw(win, 9, 2, "[Enter] Start selected user");
    mvwprintw(win, 10, 2, "[P] Start all suppliers");
    mvwprintw(win, 11, 2, "[O] Start all retailers");
    mvwprintw(win, 12, 2, "[L] Set operation limit");
    mvwprintw(win, 13, 2, "[X] Stop simulation");
    mvwprintw(win, 14, 2, "[C] Reset runtime");
    mvwprintw(win, 15, 2, "[Tab] View switch");
    mvwprintw(win, 16, 2, "[Q] Quit");

    draw_title(win, 18, 2, "SAVED SUPPLIERS / RETAILERS", 4);

    pthread_mutex_lock(&users_mutex);
    int start = selected_user > 8 ? selected_user - 8 : 0;
    int end = start + 12;
    if (end > user_count) end = user_count;

    for (int i = start; i < end; i++) {
        if (i == selected_user) wattron(win, A_REVERSE);
        mvwprintw(win, 19 + (i - start), 2, "%-14s %-8s ",
                  active_users[i].name,
                  active_users[i].is_supplier ? "Supplier" : "Retailer");
        if (active_users[i].running) {
            wattron(win, COLOR_PAIR(2) | A_BOLD);
            wprintw(win, "RUNNING");
            wattroff(win, COLOR_PAIR(2) | A_BOLD);
        } else {
            wattron(win, COLOR_PAIR(3));
            wprintw(win, "READY");
            wattroff(win, COLOR_PAIR(3));
        }
        if (i == selected_user) wattroff(win, A_REVERSE);
    }

    if (user_count == 0) {
        mvwprintw(win, 19, 2, "No suppliers/retailers yet.");
    }
    pthread_mutex_unlock(&users_mutex);

    draw_title(win, getmaxy(win) - 4, 2, "STATUS", 4);
    mvwprintw(win, getmaxy(win) - 2, 2, "%.36s", status_message);

    wrefresh(win);
}

static void draw_dashboard(WINDOW *win, const Warehouse *snap) {
    werase(win);
    box(win, 0, 0);

    draw_title(win, 1, 2, "LIVE PRODUCER-CONSUMER + AI DASHBOARD", 1);
    mvwprintw(win, 2, 2, "Dashboard");
    if (selected_tab == 0) wattron(win, A_REVERSE);
    mvwprintw(win, 2, 14, " Overview ");
    if (selected_tab == 0) wattroff(win, A_REVERSE);
    if (selected_tab == 1) wattron(win, A_REVERSE);
    mvwprintw(win, 2, 25, " AI Learning ");
    if (selected_tab == 1) wattroff(win, A_REVERSE);
    if (selected_tab == 2) wattron(win, A_REVERSE);
    mvwprintw(win, 2, 39, " Logs ");
    if (selected_tab == 2) wattroff(win, A_REVERSE);

    if (snap->simulation_running) {
        wattron(win, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(win, 1, getmaxx(win) - 24, "[ RUNNING ]");
        wattroff(win, COLOR_PAIR(2) | A_BOLD);
    } else {
        wattron(win, COLOR_PAIR(5) | A_BOLD);
        mvwprintw(win, 1, getmaxx(win) - 24, "[ STOPPED ]");
        wattroff(win, COLOR_PAIR(5) | A_BOLD);
    }

    if (selected_tab == 2) {
        draw_title(win, 4, 2, "RECENT SYSTEM LOG", 4);
        char lines[8][140];
        int logs = load_recent_logs(lines, 8);
        if (logs == 0) {
            mvwprintw(win, 6, 4, "No log entries yet. Start supplier and retailer threads.");
        } else {
            for (int i = 0; i < logs; i++) {
                mvwprintw(win, 6 + i, 4, "%.110s", lines[i]);
            }
        }
        draw_title(win, 17, 2, "LATEST EVENT", 4);
        mvwprintw(win, 18, 4, "%.100s", snap->last_event);
        wrefresh(win);
        return;
    }

    draw_buffer(win, 4, snap);

    int running_suppliers, running_retailers;
    get_running_counts(&running_suppliers, &running_retailers);

    draw_title(win, 8, 2, "THREAD ACTIVITY", 4);
    draw_metric(win, 9, 4, "Produced", snap->produced_count, 2);
    draw_metric(win, 10, 4, "Consumed", snap->consumed_count, 6);
    draw_metric(win, 11, 4, "Total Ops", snap->total_operations, 1);
    mvwprintw(win, 12, 4, "Limit: %s",
              snap->operation_limit > 0 ? "Enabled" : "Unlimited");
    if (snap->operation_limit > 0) wprintw(win, " (%d)", snap->operation_limit);
    mvwprintw(win, 13, 4, "Running suppliers: %d", running_suppliers);
    mvwprintw(win, 14, 4, "Running retailers: %d", running_retailers);

    draw_title(win, 8, 38, "WAITING / STARVATION SIGNALS", 4);
    draw_metric(win, 9, 40, "Supplier Waiting", snap->supplier_waiting, 5);
    draw_metric(win, 10, 40, "Retailer Waiting", snap->retailer_waiting, 5);
    draw_metric(win, 11, 40, "Supplier Blocks", snap->supplier_block_cycles, 5);
    draw_metric(win, 12, 40, "Retailer Blocks", snap->retailer_block_cycles, 5);

    draw_title(win, 16, 2, "AI SCHEDULER DECISION", 4);
    mvwprintw(win, 17, 4, "State  ");
    wattron(win, COLOR_PAIR(4) | A_BOLD);
    wprintw(win, "%s", ai_state_name(snap->last_ai_state));
    wattroff(win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(win, 18, 4, "Action ");
    wattron(win, COLOR_PAIR(7) | A_BOLD);
    wprintw(win, "[ %s ]", ai_action_name(snap->last_ai_action));
    wattroff(win, COLOR_PAIR(7) | A_BOLD);
    mvwprintw(win, 19, 4, "Reward %d      Q-value %.2f", snap->last_ai_reward, snap->last_ai_q_value);
    mvwprintw(win, 20, 4, "Producer Delay ");
    draw_bar(win, 20, 20, 12, 4 - snap->producer_delay, 3, 2);
    wprintw(win, " %d sec", snap->producer_delay);
    mvwprintw(win, 21, 4, "Retailer Delay ");
    draw_bar(win, 21, 20, 12, 4 - snap->retailer_delay, 3, 6);
    wprintw(win, " %d sec", snap->retailer_delay);
    mvwprintw(win, 22, 4, "Reason: %.90s", snap->last_ai_reason);

    if (selected_tab == 0) {
        draw_title(win, 24, 2, "LATEST EVENT", 4);
        mvwprintw(win, 25, 4, "%.100s", snap->last_event);
        if (show_help) {
            mvwprintw(win, getmaxy(win) - 3, 2,
                      "Add users on the left, start them, and watch AI change delays.");
            mvwprintw(win, getmaxy(win) - 2, 2,
                      "Use Tab to view AI learning table or recent system logs.");
        }
        wrefresh(win);
        return;
    }

    int table_row = 24;
    mvwprintw(win, table_row, 2, "Q-Learning Table");
    mvwprintw(win, table_row + 1, 2, "%-34s %9s %9s %9s", "State", "Supplier", "Retailer", "Balance");
    for (int i = 0; i < NUM_STATES && table_row + 2 + i < getmaxy(win) - 2; i++) {
        if (i == snap->last_ai_state) wattron(win, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(win, table_row + 2 + i, 2, "%-34s %9.2f %9.2f %9.2f",
                  ai_state_name(i),
                  snap->q_table[i][ACTION_BOOST_SUPPLIERS],
                  snap->q_table[i][ACTION_BOOST_RETAILERS],
                  snap->q_table[i][ACTION_BALANCE]);
        if (i == snap->last_ai_state) wattroff(win, COLOR_PAIR(4) | A_BOLD);
    }

    wrefresh(win);
}

void run_ncurses_interface(void) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    timeout(200);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_WHITE, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_RED, COLOR_BLACK);
        init_pair(6, COLOR_BLUE, COLOR_BLACK);
        init_pair(7, COLOR_MAGENTA, COLOR_BLACK);
    }

    while (1) {
        int height, width;
        getmaxyx(stdscr, height, width);

        if (height < 24 || width < 100) {
            erase();
            mvprintw(1, 2, "Please enlarge the terminal window for the full interface.");
            mvprintw(2, 2, "Current size: %d rows x %d columns. Needed: 24 rows x 100 columns.", height, width);
            refresh();
            int ch = getch();
            if (ch == 'q' || ch == 'Q') break;
            continue;
        }

        WINDOW *left = newwin(height, PANEL_W, 0, 0);
        WINDOW *right = newwin(height, width - PANEL_W, 0, PANEL_W);

        Warehouse snap;
        pthread_mutex_lock(&warehouse->mutex);
        snap = *warehouse;
        pthread_mutex_unlock(&warehouse->mutex);

        draw_left_panel(left);
        draw_dashboard(right, &snap);

        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == KEY_UP) {
            if (selected_user > 0) selected_user--;
        } else if (ch == KEY_DOWN) {
            pthread_mutex_lock(&users_mutex);
            if (selected_user < user_count - 1) selected_user++;
            pthread_mutex_unlock(&users_mutex);
        } else if (ch == '\t') {
            selected_tab = (selected_tab + 1) % 3;
        } else if (ch == 'a' || ch == 'A') {
            prompt_admin_login();
        } else if (!admin_logged_in) {
            if (ch != ERR) set_status("Please login as admin first.");
        } else if (ch == 's' || ch == 'S') {
            prompt_add_user(1);
        } else if (ch == 'r' || ch == 'R') {
            prompt_add_user(0);
        } else if (ch == '\n' || ch == KEY_ENTER || ch == 10) {
            start_selected_user();
        } else if (ch == 'l' || ch == 'L') {
            prompt_operation_limit();
        } else if (ch == 'p' || ch == 'P') {
            start_users_by_role(1);
        } else if (ch == 'o' || ch == 'O') {
            start_users_by_role(0);
        } else if (ch == 'x' || ch == 'X') {
            request_simulation_stop();
            set_status("Simulation stopped.");
        } else if (ch == 'c' || ch == 'C') {
            if (any_user_running()) {
                set_status("Stop simulation first, wait a moment, then reset.");
            } else {
                reset_warehouse_runtime();
                set_status("Runtime counters reset.");
            }
        } else if (ch == 'h' || ch == 'H') {
            show_help = !show_help;
        }

        delwin(left);
        delwin(right);
    }

    endwin();
}
