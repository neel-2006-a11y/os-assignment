#include <pthread.h>
#include <chrono>
#include "structs.h"
#include <string>

// Scheduler toggle:
// true  → SJF: tail-enders enter in descending ID order (worst batter first)
// false → FCFS: tail-enders enter in ascending ID order (natural order)
bool sjf_scheduler = false;

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
int NUM_OVERS = 20;
bool match_end = false;
int current_team = 0;
int scheduled_bowler_id = 0;

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
int total_runs = 0;
int innings = 1;
int team1_score = 0;
int team2_score = 0;
std::string last_event = "";
int curr_ball = 0;
int curr_over = 0;

BallState ball_state = INITIAL;

// bowler
int prev_bowler_id = 0, curr_bowler_id = 0;
bool new_bowler = false;

// batsman
int striker_id = 0, non_striker_id = 0;
int batsmen_chosen = 0;
int next_bat_order = 1;
bool striker_side_start_run = false, non_striker_side_start_run = false;
bool striker_side_end_run = false, non_striker_side_end_run = false;
std::chrono::steady_clock::time_point striker_side_reach_time, non_striker_side_reach_time;

// fielder
int catcher_id = 0;
std::chrono::steady_clock::time_point fielder_ball_reach_time;
PitchSide throw_side;

// Deadlock Detection (RAG)
pthread_mutex_t rag_lock            = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  crease_cond         = PTHREAD_COND_INITIALIZER;
int  crease_held_by[2]              = {0, 0};
int  batsman_wants[12]              = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
bool deadlock_detected              = false;
int  deadlock_victim_id             = 0;
bool run_collision_possible         = false;
