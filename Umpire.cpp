#include <pthread.h>
#include <iostream>

#include "Umpire.h"
#include "shared.h"
#include "players.h"
#include "scheduler.h"



void match_start(){
    pthread_mutex_lock(&umpire_lock);
    match = true;
    pthread_cond_broadcast(&waiting_for_match);
    pthread_mutex_unlock(&umpire_lock);
}

void wait_new_bowler_umpire(){
    pthread_mutex_lock(&pitch_lock);
    while(!new_bowler){
        pthread_cond_wait(&waiting_for_new_bowler, &pitch_lock);
    }
    pthread_mutex_unlock(&pitch_lock);
}

void wait_two_batsmen(){
    pthread_mutex_lock(&pitch_lock);

    while(batsmen_chosen < 2 || striker_id == 0 || non_striker_id == 0){
        pthread_cond_wait(&waiting_for_batsmen_chosen, &pitch_lock);
    }

    pthread_mutex_unlock(&pitch_lock);
}

void wait_bowler_chosen(){
    pthread_mutex_lock(&pitch_lock);
    while(curr_bowler_id == 0){
        pthread_cond_wait(&waiting_for_bowler_chosen, &pitch_lock);
    }
    pthread_mutex_unlock(&pitch_lock);
}

void signal_new_bowler(){
    pthread_mutex_lock(&pitch_lock);
    pthread_cond_broadcast(&waiting_for_new_bowler);
    pthread_mutex_unlock(&pitch_lock);
}

void wait_for_fielders_to_receive_new_bowler(){
    pthread_mutex_lock(&umpire_lock);
    while(fielders_received_bowler<NUM_FIELDERS){
        pthread_cond_wait(&waiting_for_fielders_received_bowler, &umpire_lock);
    }
    fielders_received_bowler = 0;
    new_bowler = false;
    pthread_mutex_unlock(&umpire_lock);
}

void set_next_ball_ready(){
    // Initialise RAG for this delivery.
    // 10% chance: both batsmen will use hold-and-wait → guaranteed deadlock.
    pthread_mutex_lock(&rag_lock);
    crease_held_by[0]      = striker_id;
    crease_held_by[1]      = non_striker_id;
    for(int i = 0; i <= 11; i++) batsman_wants[i] = -1;
    deadlock_detected      = false;
    deadlock_victim_id     = 0;
    run_collision_possible = (rand() % 80 == 0);   // ~1.25% per ball → ~1-2 per inning
    if (run_collision_possible)
        std::cout << "[RAG] Collision scenario this delivery — hold-and-wait active\n";
    pthread_mutex_unlock(&rag_lock);

    pthread_mutex_lock(&umpire_lock);
    next_ball_ready = true;
    ball_state = INITIAL;
    pthread_cond_broadcast(&waiting_for_next_ball);
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_all_players_to_play_ball(){
    pthread_mutex_lock(&umpire_lock);
    while(players_played_ball < (NUM_FIELDERS + 2)){
        pthread_cond_wait(&waiting_for_players_play_ball, &umpire_lock);
    }
    next_ball_ready = false;
    players_played_ball = 0;
    balls_played_in_over++;
    pthread_mutex_unlock(&umpire_lock);
}

void get_new_bowler(int over_runs, int over_wickets){
    int bowling_team = 1 - current_team;
    // Ask scheduler for the next bowler (RR / Priority) and save outgoing stats
    int next_id = scheduler_next_bowler(bowling_team, curr_bowler_id,
                                        over_runs, over_wickets);
    pthread_mutex_lock(&pitch_lock);
    prev_bowler_id    = curr_bowler_id;
    scheduled_bowler_id = next_id;     // only this bowler may take the spot
    curr_bowler_id    = 0;
    pthread_cond_broadcast(&waiting_for_bowler_spot);
    pthread_mutex_unlock(&pitch_lock);
}

void get_new_batsman(PitchSide side){
    pthread_mutex_lock(&pitch_lock);
    if(side == STRIKER)striker_id = 0;
    else non_striker_id = 0;
    pthread_cond_broadcast(&waiting_for_batsman_spot);
    pthread_mutex_unlock(&pitch_lock);
}

void signal_result_of_ball(){
    pthread_mutex_lock(&umpire_lock);
    result_of_ball_ready = true;
    pthread_cond_broadcast(&waiting_for_result_of_ball);
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_players_to_receive_result_of_ball(){
    pthread_mutex_lock(&umpire_lock);
    while(players_received_result_of_ball < NUM_FIELDERS+2){
        pthread_cond_wait(&waiting_for_players_to_receive_result_of_ball, &umpire_lock);
    }
    players_received_result_of_ball = 0;
    result_of_ball_ready = false;
    catcher_id = 0;
    pthread_mutex_unlock(&umpire_lock);

    // Reset RAG state for the next delivery.
    pthread_mutex_lock(&rag_lock);
    crease_held_by[0]      = 0;
    crease_held_by[1]      = 0;
    deadlock_detected      = false;
    deadlock_victim_id     = 0;
    run_collision_possible = false;
    pthread_mutex_unlock(&rag_lock);
}

void signal_sleeping_fielder(){
    pthread_mutex_lock(&pitch_lock);
    pthread_cond_broadcast(&waiting_for_new_bowler);
    pthread_mutex_unlock(&pitch_lock);
}

void signal_sleeping_bowlers(){
    pthread_mutex_lock(&pitch_lock);
    pthread_cond_broadcast(&waiting_for_bowler_spot);
    pthread_mutex_unlock(&pitch_lock);
}

void signal_sleeping_batsman(){
    pthread_mutex_lock(&pitch_lock);
    pthread_cond_broadcast(&waiting_for_batsman_spot);
    pthread_mutex_unlock(&pitch_lock);

    // Also wake any batsmen blocked in hold-and-wait inside attempt_run().
    // this will help threads waiting on crease_cond to see match_end or hang.
    pthread_mutex_lock(&rag_lock);
    pthread_cond_broadcast(&crease_cond);
    pthread_mutex_unlock(&rag_lock);
}

void* Umpire(void* arg){
    match_start();

    while(!match_end){
        int over_runs    = 0;   // runs scored this over (for scheduler context switch)
        int over_wickets = 0;   // wickets this over
        wait_new_bowler_umpire();
        while(true){
            wait_two_batsmen();
            if(new_bowler){
                wait_for_fielders_to_receive_new_bowler();
                int bt = 1 - current_team;
                std::cout << "\n── Over " << (over_finished + 1)
                          << ": " << player_name(bt, curr_bowler_id) << " to bowl ──\n";
            }

            set_next_ball_ready();

            wait_for_all_players_to_play_ball();
            curr_ball++;

            if(curr_ball > 6){
             curr_ball = 1;
             curr_over++;
            }

            // Snapshot IDs now — before result clears striker/bowler/catcher slots
          int snap_striker     = striker_id;
          int snap_non_striker = non_striker_id;
          int snap_bowler      = curr_bowler_id;
          int snap_catcher     = catcher_id;

            // no player accessing next_ball, all next waiting for result
          int r = rand() % 100;
          int bowling_team = 1 - current_team;

          bool spec = is_death_specialist(bowling_team, snap_bowler);
          bool tail = is_tail_ender(current_team, snap_striker);

          // 4-way probability table  [BOWLED, CAUGHT, dot, 1R, 2R, 3R, 4, SIX]
          // Death specialist bowling: slightly higher wickets, slightly lower runs
          int tb, tc, td, t1, t2, t3, t4;   // cumulative thresholds
          if (tail && spec) {
              tb=12; tc=22; td=55; t1=75; t2=87; t3=94; t4=97; // 12+10+33+20+12+7+3+3
          } else if (tail) {
              tb=10; tc=18; td=50; t1=72; t2=84; t3=91; t4=97; // 10+8+32+22+12+7+6+3
          } else if (spec) {
              tb=4;  tc=8;  td=36; t1=59; t2=72; t3=81; t4=93; // 4+4+28+23+13+9+12+7
          } else {
              tb=2;  tc=4;  td=30; t1=55; t2=70; t3=80; t4=92; // 2+2+26+25+15+10+12+8
          }

if(r < tb){
    wickets++; over_wickets++;
    batsmen_chosen--;
    get_new_batsman(STRIKER);
    last_event = "BOWLED";
}
else if(r < tc){
    wickets++; over_wickets++;
    batsmen_chosen--;
    get_new_batsman(STRIKER);
    last_event = "CAUGHT by " + player_name(bowling_team, snap_catcher);
}
else if(r < td){ last_event = "dot"; }
else if(r < t1){ total_runs += 1; over_runs += 1; last_event = "1 run"; }
else if(r < t2){ total_runs += 2; over_runs += 2; last_event = "2 runs"; }
else if(r < t3){ total_runs += 3; over_runs += 3; last_event = "3 runs"; }
else if(r < t4){ total_runs += 4; over_runs += 4; last_event = "FOUR"; }
else           { total_runs += 6; over_runs += 6; last_event = "SIX"; }
            // make result
            // run_out
            
          // ── Deadlock-based Run-Out detection ─────────────────────────────────
          // If the DeadlockDetector identified a victim this delivery and the ball
          // didn't already result in BOWLED/CAUGHT, declare that batsman Run Out.
          if(deadlock_victim_id != 0
                && last_event != "BOWLED"
                && last_event.rfind("CAUGHT", 0) != 0) {
              PitchSide victim_side =
                  (deadlock_victim_id == snap_striker) ? STRIKER : NON_STRIKER;
              wickets++; over_wickets++;
              batsmen_chosen--;
              get_new_batsman(victim_side);
              last_event = "RUN OUT (deadlock) "
                         + player_name(current_team, deadlock_victim_id)
                         + " | throw: " + player_name(bowling_team, snap_catcher);
          }

            // change over — flags only here; print + scheduler call happen after commentary
            if(balls_played_in_over == 6){
                balls_played_in_over = 0;
                new_over = true;
                over_finished++;
            }

            // Inning 2: end as soon as chasing team passes the target
            if(current_team == 1 && total_runs > team1_score){
                match_end = true;
                signal_sleeping_fielder();
                signal_sleeping_bowlers();
                signal_sleeping_batsman();
            }

            if(!match_end && (wickets==10 || over_finished == NUM_OVERS)){
                match_end = true;
                signal_sleeping_fielder();
                signal_sleeping_bowlers();
                signal_sleeping_batsman();
            }

            pthread_mutex_lock(&print_lock);

          // Use snapshots so wicket/over-change can't race with the print
          int batting_team = current_team;

          std::cout << "[" << curr_over << "." << curr_ball << "] "
          << player_name(bowling_team, snap_bowler)
          << " to " << player_name(batting_team, snap_striker)

          << " | Score: " << total_runs << "/" << wickets
          << " | " << last_event << "\n";

// 1. swap for odd runs
if(last_event == "1 run" || last_event == "3 runs"){
    int temp = striker_id;
    striker_id = non_striker_id;
    non_striker_id = temp;
}

// 2. swap at end of over
if(new_over){
    int temp = striker_id;
    striker_id = non_striker_id;
    non_striker_id = temp;
}
            pthread_mutex_unlock(&print_lock);

            // end-of-over summary + scheduler call AFTER ball commentary is printed
            if(new_over){
                int bt = 1 - current_team;
                std::cout << "── End of over " << over_finished
                          << ": " << player_name(bt, snap_bowler)
                          << " conceded " << over_runs << " run(s) ──\n";
                get_new_bowler(over_runs, over_wickets);
                over_runs = 0; over_wickets = 0;
            }

            // signal result of ball
            signal_result_of_ball();
            wait_for_players_to_receive_result_of_ball();
            // reset list
            if(new_over || match_end){
                new_over = false;
                break;
            }

            striker_side_start_run = false;
            striker_side_end_run = false;
            non_striker_side_start_run = false;
            non_striker_side_end_run = false;
        }
    }

    const std::string &batting = (current_team == 0) ? TEAM1_NAME : TEAM2_NAME;
    std::cout << "\n" << batting << " innings complete: "
              << total_runs << "/" << wickets << "\n";
    return NULL;
}