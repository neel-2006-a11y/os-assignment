#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <chrono>
#include "structs.h"
#include <string>

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

// ── Scheduler toggle ──────────────────────────────────────────────────────────
// true means SJF order: highest ID comes in first decending order (11→10→9)
// false means FCFS order: natural ascending ID order (1→2→3)
extern bool sjf_scheduler;

// Shared state
// global related states
extern bool match;
extern int NUM_FIELDERS;
extern int NUM_OVERS;
extern bool match_end;
extern int current_team;         // 0 = India batting, 1 = Pakistan batting
extern int scheduled_bowler_id;  // 0 = any eligible, >0 = only this bowler

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
extern int total_runs;
extern int innings;
extern int team1_score;
extern int team2_score;
extern std::string last_event;
extern int curr_ball;
extern int curr_over;


extern BallState ball_state;

// bowler
extern int prev_bowler_id, curr_bowler_id;
extern bool new_bowler;

// batsman
extern int striker_id, non_striker_id;
extern int batsmen_chosen;
extern int next_bat_order;   // enforces batting order 1→11
extern bool striker_side_start_run, non_striker_side_start_run;
extern bool striker_side_end_run, non_striker_side_end_run;
extern std::chrono::steady_clock::time_point striker_side_reach_time, non_striker_side_reach_time;

// fielder
extern int catcher_id;
extern std::chrono::steady_clock::time_point fielder_ball_reach_time;
extern PitchSide throw_side;

// ── Deadlock Detection (Resource Allocation Graph) ────────────────────────────
// crease[0] = striker end, crease[1] = non-striker end
// crease_held_by[i]  → batsman ID currently occupying that end (0 = empty)
// batsman_wants[id]  → crease index the batsman is blocked waiting for (-1 = none)
// run_collision_possible → per-ball flag: both batsmen use hold-and-wait this delivery
extern pthread_mutex_t rag_lock;
extern pthread_cond_t  crease_cond;
extern int  crease_held_by[2];
extern int  batsman_wants[12];
extern bool deadlock_detected;
extern int  deadlock_victim_id;
extern bool run_collision_possible;
