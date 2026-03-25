#include <pthread.h>
#include <chrono>
#include "structs.h"

// Mutex
pthread_mutex_t pitch_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t umpire_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

// Condition variables
pthread_cond_t waiting_for_match = PTHREAD_COND_INITIALIZER;
pthread_cond_t waiting_for_batsmen_chosen = PTHREAD_COND_INITIALIZER;
pthread_cond_t waiting_for_bowler_chosen = PTHREAD_COND_INITIALIZER;
pthread_cond_t waiting_for_new_bowler = PTHREAD_COND_INITIALIZER;
pthread_cond_t waiting_for_fielders_received_bowler = PTHREAD_COND_INITIALIZER;
pthread_cond_t waiting_for_next_ball = PTHREAD_COND_INITIALIZER;
pthread_cond_t waiting_for_players_play_ball = PTHREAD_COND_INITIALIZER;
pthread_cond_t waiting_for_result_of_ball = PTHREAD_COND_INITIALIZER;
pthread_cond_t waiting_for_players_to_receive_result_of_ball = PTHREAD_COND_INITIALIZER;

pthread_cond_t waiting_for_bowler_spot = PTHREAD_COND_INITIALIZER;

pthread_cond_t waiting_for_ball_thrown = PTHREAD_COND_INITIALIZER;
pthread_cond_t waiting_for_ball_hit = PTHREAD_COND_INITIALIZER;
pthread_cond_t waiting_for_batsman_spot = PTHREAD_COND_INITIALIZER;

// Shared state
// global related states
bool match = false;
int NUM_FIELDERS = 11;
int NUM_OVERS = 5;
bool match_end = false;

// umpire
int fielders_received_bowler = 0;
bool next_ball_ready = false;
int players_played_ball = 0;
int balls_played_in_over = 0;
bool result_of_ball_ready = false;
int players_received_result_of_ball = 0;
bool new_over = false;
int over_finished = 0;
int wickets = 0;


BallState ball_state = INITIAL;

// bowler
int prev_bowler_id = 0, curr_bowler_id = 0;
bool new_bowler = false;

// batsman
int striker_id = 0, non_striker_id = 0;
int batsmen_chosen = 0;
bool striker_side_start_run = false, non_striker_side_start_run = false;
bool striker_side_end_run = false, non_striker_side_end_run = false;
std::chrono::steady_clock::time_point striker_side_reach_time, non_striker_side_reach_time;

// fielder
int catcher_id = 0;
std::chrono::steady_clock::time_point fielder_ball_reach_time;
PitchSide throw_side;
