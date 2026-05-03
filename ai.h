#ifndef AI_H
#define AI_H

#include "warehouse.h"

#define NUM_STATES 6
#define NUM_ACTIONS 3

// Actions
#define ACTION_BOOST_SUPPLIERS 0
#define ACTION_BOOST_RETAILERS 1
#define ACTION_BALANCE 2

// States
#define STATE_LOW_NO_STARVATION 0
#define STATE_LOW_STARVATION 1
#define STATE_BALANCED_NO_STARVATION 2
#define STATE_BALANCED_STARVATION 3
#define STATE_HIGH_NO_STARVATION 4
#define STATE_HIGH_STARVATION 5

void *ai_scheduler_thread(void *arg);
int get_ai_state(void);
int choose_action(int state);
int compute_reward(int state, int action);
void apply_action(int action);
void update_q_table(int state, int action, int reward, int next_state);
const char *ai_state_name(int state);
const char *ai_action_name(int action);

#endif
