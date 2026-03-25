#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <chrono>
#include "structs.h"

// Mutex
extern pthread_mutex_t pitch_lock;
extern pthread_mutex_t umpire_lock;
extern pthread_mutex_t print_lock;

// Condition variables
extern pthread_cond_t waiting_for_match;
extern pthread_cond_t waiting_for_batsmen_chosen;
extern pthread_cond_t waiting_for_bowler_chosen;
extern pthread_cond_t waiting_for_new_bowler;
extern pthread_cond_t waiting_for_fielders_received_bowler;
extern pthread_cond_t waiting_for_next_ball;
extern pthread_cond_t waiting_for_players_play_ball;
extern pthread_cond_t waiting_for_result_of_ball;
extern pthread_cond_t waiting_for_players_to_receive_result_of_ball;

extern pthread_cond_t waiting_for_bowler_spot;

extern pthread_cond_t waiting_for_ball_thrown;
extern pthread_cond_t waiting_for_ball_hit;
extern pthread_cond_t waiting_for_batsman_spot;

// Shared state
// global related states
extern bool match;
extern int NUM_FIELDERS;
extern int NUM_OVERS;
extern bool match_end;

// umpire
extern int fielders_received_bowler;
extern bool next_ball_ready;
extern int players_played_ball;
extern int balls_played_in_over;
extern bool result_of_ball_ready;
extern int players_received_result_of_ball;
extern bool new_over;
extern int over_finished;
extern int wickets;


extern BallState ball_state;

// bowler
extern int prev_bowler_id, curr_bowler_id;
extern bool new_bowler;

// batsman
extern int striker_id, non_striker_id;
extern int batsmen_chosen;
extern bool striker_side_start_run, non_striker_side_start_run;
extern bool striker_side_end_run, non_striker_side_end_run;
extern std::chrono::steady_clock::time_point striker_side_reach_time, non_striker_side_reach_time;

// fielder
extern int catcher_id;
extern std::chrono::steady_clock::time_point fielder_ball_reach_time;
extern PitchSide throw_side;
