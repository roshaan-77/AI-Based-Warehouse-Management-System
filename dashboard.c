#include "warehouse.h"
#include "ai.h"

#include <fcntl.h>
#include <ncurses.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

Warehouse *warehouse;

static void draw_buffer(int row, const Warehouse *snap) {
    mvprintw(row, 2, "Warehouse Buffer");
    mvprintw(row + 1, 2, "[");

    for (int i = 0; i < BUFFER_SIZE; i++) {
        int occupied = i < snap->buffer_count;
        if (occupied) attron(COLOR_PAIR(2) | A_BOLD);
        else attron(COLOR_PAIR(3));

        printw(" %s ", occupied ? "X" : "_");

        if (occupied) attroff(COLOR_PAIR(2) | A_BOLD);
        else attroff(COLOR_PAIR(3));
    }

    printw("]  %d/%d", snap->buffer_count, BUFFER_SIZE);
}

static void draw_q_table(int row, const Warehouse *snap) {
    mvprintw(row, 2, "Q-Learning Table");
    mvprintw(row + 1, 2, "%-34s %10s %10s %10s",
             "State", "Supplier", "Retailer", "Balance");

    for (int i = 0; i < NUM_STATES; i++) {
        if (i == snap->last_ai_state) attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(row + 2 + i, 2, "%-34s %10.2f %10.2f %10.2f",
                 ai_state_name(i),
                 snap->q_table[i][ACTION_BOOST_SUPPLIERS],
                 snap->q_table[i][ACTION_BOOST_RETAILERS],
                 snap->q_table[i][ACTION_BALANCE]);
        if (i == snap->last_ai_state) attroff(COLOR_PAIR(4) | A_BOLD);
    }
}

static void draw_screen(const Warehouse *snap) {
    erase();
    box(stdscr, 0, 0);

    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(1, 3, "Adaptive Warehouse Management Dashboard");
    attroff(COLOR_PAIR(1) | A_BOLD);

    mvprintw(2, 3, "Press S to stop simulation | Q to quit dashboard");

    draw_buffer(4, snap);

    mvprintw(7, 2, "Producer-Consumer Activity");
    mvprintw(8, 4, "Produced Items : %d", snap->produced_count);
    mvprintw(9, 4, "Consumed Items : %d", snap->consumed_count);
    mvprintw(10, 4, "Total Ops      : %d", snap->total_operations);
    mvprintw(11, 4, "Operation Limit: %s",
             snap->operation_limit > 0 ? "Enabled" : "Unlimited");
    if (snap->operation_limit > 0) {
        mvprintw(11, 22, "(%d)", snap->operation_limit);
    }

    mvprintw(8, 42, "Supplier Waiting : %d", snap->supplier_waiting);
    mvprintw(9, 42, "Retailer Waiting : %d", snap->retailer_waiting);
    mvprintw(10, 42, "Supplier Blocks  : %d", snap->supplier_block_cycles);
    mvprintw(11, 42, "Retailer Blocks  : %d", snap->retailer_block_cycles);

    mvprintw(13, 2, "AI Scheduler");
    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(14, 4, "State : %s", ai_state_name(snap->last_ai_state));
    mvprintw(15, 4, "Action: %s", ai_action_name(snap->last_ai_action));
    attroff(COLOR_PAIR(4) | A_BOLD);
    mvprintw(16, 4, "Reward: %d", snap->last_ai_reward);
    mvprintw(17, 4, "Q Now : %.2f", snap->last_ai_q_value);
    mvprintw(18, 4, "Producer Delay: %d sec", snap->producer_delay);
    mvprintw(19, 4, "Retailer Delay: %d sec", snap->retailer_delay);
    mvprintw(20, 4, "Reason: %.100s", snap->last_ai_reason);

    mvprintw(22, 2, "Latest Event");
    mvprintw(23, 4, "%.120s", snap->last_event);

    draw_q_table(25, snap);

    if (!snap->simulation_running) {
        attron(COLOR_PAIR(5) | A_BOLD);
        mvprintw(3, 55, "SIMULATION STOPPED");
        attroff(COLOR_PAIR(5) | A_BOLD);
    } else {
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(3, 55, "SIMULATION RUNNING");
        attroff(COLOR_PAIR(2) | A_BOLD);
    }

    refresh();
}

static void stop_simulation_from_dashboard(void) {
    pthread_mutex_lock(&warehouse->mutex);
    warehouse->simulation_running = 0;
    snprintf(warehouse->last_event, sizeof(warehouse->last_event),
             "Simulation stopped from dashboard");
    pthread_mutex_unlock(&warehouse->mutex);

    for (int i = 0; i < BUFFER_SIZE + 8; i++) {
        sem_post(&warehouse->empty);
        sem_post(&warehouse->full);
    }
}

int main(void) {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        fprintf(stderr, "Warehouse shared memory not found. Start ./warehouse_system first.\n");
        return 1;
    }

    warehouse = mmap(0, sizeof(Warehouse), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (warehouse == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_WHITE, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_RED, COLOR_BLACK);
    }

    while (1) {
        Warehouse snap;

        pthread_mutex_lock(&warehouse->mutex);
        snap = *warehouse;
        pthread_mutex_unlock(&warehouse->mutex);

        draw_screen(&snap);

        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;
        if (ch == 's' || ch == 'S') stop_simulation_from_dashboard();

        usleep(250000);
    }

    endwin();
    munmap(warehouse, sizeof(Warehouse));
    close(shm_fd);
    return 0;
}
