#include "ai.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static float alpha = 0.2f;
static float gamma = 0.8f;
static float epsilon = 0.2f;

int get_ai_state(void) {
    int count = warehouse->buffer_count;
    int level;

    if (count <= 1)
        level = 0;   // low
    else if (count >= BUFFER_SIZE - 1)
        level = 2;   // high
    else
        level = 1;   // balanced

    if ((level == 0 && warehouse->retailer_waiting > 0) ||
        (level == 2 && warehouse->supplier_waiting > 0)) {
        warehouse->starvation_flag = 1;
    } else {
        warehouse->starvation_flag = 0;
    }

    if (level == 0 && warehouse->starvation_flag == 0) return STATE_LOW_NO_STARVATION;
    if (level == 0 && warehouse->starvation_flag == 1) return STATE_LOW_STARVATION;
    if (level == 1 && warehouse->starvation_flag == 0) return STATE_BALANCED_NO_STARVATION;
    if (level == 1 && warehouse->starvation_flag == 1) return STATE_BALANCED_STARVATION;
    if (level == 2 && warehouse->starvation_flag == 0) return STATE_HIGH_NO_STARVATION;
    return STATE_HIGH_STARVATION;
}

int choose_action(int state) {
    float r = (float)rand() / RAND_MAX;

    if (r < epsilon) {
        return rand() % NUM_ACTIONS;
    }

    int best_action = 0;
    float best_value = warehouse->q_table[state][0];

    for (int i = 1; i < NUM_ACTIONS; i++) {
        if (warehouse->q_table[state][i] > best_value) {
            best_value = warehouse->q_table[state][i];
            best_action = i;
        }
    }

    return best_action;
}

int compute_reward(int state, int action) {
    switch (state) {
        case STATE_LOW_NO_STARVATION:
            return (action == ACTION_BOOST_SUPPLIERS) ? 5 :
                   (action == ACTION_BALANCE) ? 2 : -3;

        case STATE_LOW_STARVATION:
            return (action == ACTION_BOOST_SUPPLIERS) ? 8 : -5;

        case STATE_BALANCED_NO_STARVATION:
            return (action == ACTION_BALANCE) ? 6 : 1;

        case STATE_BALANCED_STARVATION:
            return 2;

        case STATE_HIGH_NO_STARVATION:
            return (action == ACTION_BOOST_RETAILERS) ? 5 :
                   (action == ACTION_BALANCE) ? 2 : -3;

        case STATE_HIGH_STARVATION:
            return (action == ACTION_BOOST_RETAILERS) ? 8 : -5;

        default:
            return 0;
    }
}

void apply_action(int action) {
    switch (action) {
        case ACTION_BOOST_SUPPLIERS:
            warehouse->producer_delay = 1;
            warehouse->retailer_delay = 3;
            break;

        case ACTION_BOOST_RETAILERS:
            warehouse->producer_delay = 3;
            warehouse->retailer_delay = 1;
            break;

        case ACTION_BALANCE:
        default:
            warehouse->producer_delay = 2;
            warehouse->retailer_delay = 2;
            break;
    }
}

void update_q_table(int state, int action, int reward, int next_state) {
    float max_next = warehouse->q_table[next_state][0];

    for (int i = 1; i < NUM_ACTIONS; i++) {
        if (warehouse->q_table[next_state][i] > max_next) {
            max_next = warehouse->q_table[next_state][i];
        }
    }

    warehouse->q_table[state][action] =
        warehouse->q_table[state][action] +
        alpha * (reward + gamma * max_next - warehouse->q_table[state][action]);
}

void *ai_scheduler_thread(void *arg) {
    srand(time(NULL) + getpid());

    while (1) {
        sleep(5);

        pthread_mutex_lock(&warehouse->mutex);

        int state = get_ai_state();
        int action = choose_action(state);
        int reward = compute_reward(state, action);

        apply_action(action);

        int next_state = get_ai_state();
        update_q_table(state, action, reward, next_state);

        int old_state = warehouse->last_ai_state;
        int old_action = warehouse->last_ai_action;

        if (old_state != state || old_action != action) {
            FILE *fp = fopen("ai_log.txt", "a");
            if (fp != NULL) {
                fprintf(fp,
                        "[AI] State=%d Action=%d Reward=%d | Buffer=%d | P_Delay=%d R_Delay=%d\n",
                        state, action, reward,
                        warehouse->buffer_count,
                        warehouse->producer_delay,
                        warehouse->retailer_delay);
                fclose(fp);
            }
        }

        warehouse->last_ai_state = state;
        warehouse->last_ai_action = action;
        warehouse->last_ai_reward = reward;

        pthread_mutex_unlock(&warehouse->mutex);
    }

    return NULL;
}