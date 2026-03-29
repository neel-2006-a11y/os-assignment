#include <pthread.h>
#include <chrono>
#include <iostream>
#include "Batsman.h"
#include "shared.h"
#include "scheduler.h"
#include "players.h"
#include "GanttLogger.h"
#include <unistd.h>

void wait_match_start_bat(){
    pthread_mutex_lock(&umpire_lock);
    while(!match){
        pthread_cond_wait(&waiting_for_match, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_batsman_spot(int id){
    pthread_mutex_lock(&pitch_lock);
    // Wait until: bat_order[position] == my ID AND a spot is free (SJF-aware)
    while(!match_end && (next_bat_order > 11 ||
          bat_order[next_bat_order] != id || (striker_id!=0 && non_striker_id!=0))){
        pthread_cond_wait(&waiting_for_batsman_spot,&pitch_lock);
    }
    if(match_end){
        pthread_mutex_unlock(&pitch_lock);
        return;
    }
    next_bat_order++;   // advance order so the next player becomes eligible
    if(striker_id == 0){
        striker_id = id;
    }else{
        non_striker_id = id;
    }
    batsmen_chosen++;
    // Wake the next batsman-in-order so they can check their turn
    pthread_cond_broadcast(&waiting_for_batsman_spot);
    if(batsmen_chosen == 2){
        pthread_cond_broadcast(&waiting_for_batsmen_chosen);
    }
    pthread_mutex_unlock(&pitch_lock);
}

void wait_for_next_ball_bat(){
    pthread_mutex_lock(&umpire_lock);
    while(!next_ball_ready){
        pthread_cond_wait(&waiting_for_next_ball, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_ball_thrown(){
    pthread_mutex_lock(&pitch_lock);
    while(ball_state!=THROWN){
        pthread_cond_wait(&waiting_for_ball_thrown, &pitch_lock);
    }
    pthread_mutex_unlock(&pitch_lock);
}

void wait_ball_in_air_bat(){
    pthread_mutex_lock(&pitch_lock);
    while(ball_state!=IN_AIR){
        pthread_cond_wait(&waiting_for_ball_hit, &pitch_lock);
    }
    pthread_mutex_unlock(&pitch_lock);
}

void bat(){
    std::string name = player_name(current_team, striker_id);
    GanttLogger::log(name, "STRIKER", "BAT_START");
    pthread_mutex_lock(&pitch_lock);
    ball_state = IN_AIR;
    pthread_cond_broadcast(&waiting_for_ball_hit);
    pthread_mutex_unlock(&pitch_lock);
    GanttLogger::log(name, "STRIKER", "BAT_END");
}

// Attempts to run from current end to the opposite end.
// If run_collision_possible is set this delivery, both batsmen use hold-and-wait
// (they keep their current crease and try to acquire the other), creating the
// exact circular-wait the DeadlockDetector thread looks for.
//
// Returns true if this batsman is the deadlock victim.
// IMPORTANT: returning true does NOT mean the thread should exit — the umpire
// may suppress the run-out (e.g. ball was also BOWLED).  The caller must rely
// on check_if_still_batsman() after the result is announced, not on this flag.
bool attempt_run(int id, PitchSide side) {
    int src = (side == STRIKER) ? 0 : 1;
    int dst = 1 - src;
    const std::string role = (side == STRIKER) ? "STRIKER" : "NON_STRIKER";
    const std::string name = player_name(current_team, id);

    GanttLogger::log(name, role, "RUN_START");

    if (!run_collision_possible) {
        // Normal delivery(no deadlock risk, simulate running with a delay).
        usleep(1000 + rand() % 2000);
        GanttLogger::log(name, role, "RUN_END");
        return false;
    }

    //Hold-and-wait path
    std::cout << "[RAG] Batsman-" << id
              << " holds End-" << (src+1)
              << ", wants End-" << (dst+1) << "\n";

    pthread_mutex_lock(&rag_lock);
    batsman_wants[id] = dst;   // RAG edge: id → crease[dst]

    // exit on match_end so the thread doesn't block forever when
    // the inning ends while batsmen are in the hold-and-wait wait loop.
    while (crease_held_by[dst] != 0 && !deadlock_detected && !match_end) {
        pthread_cond_wait(&crease_cond, &rag_lock);
    }

    if (match_end) {
        batsman_wants[id]   = -1;
        crease_held_by[src] = 0;
        pthread_cond_broadcast(&crease_cond);
        pthread_mutex_unlock(&rag_lock);
        GanttLogger::log(name, role, "RUN_END");
        return false;
    }

    if (deadlock_detected && deadlock_victim_id == id) {
        // I am the victim: release my crease and exit — Run Out (pending umpire confirm).
        std::cout << "[RAG] Batsman-" << id
                  << " victim — releasing End-" << (src+1) << "\n";
        crease_held_by[src] = 0;
        batsman_wants[id]   = -1;
        pthread_cond_broadcast(&crease_cond);   // let survivor proceed
        pthread_mutex_unlock(&rag_lock);
        GanttLogger::log(name, role, "RUN_END");
        return true;
    }

    // Survivor (or deadlock resolved and I am not victim): safely cross.
    std::cout << "[RAG] Batsman-" << id
              << " crossed to End-" << (dst+1) << "\n";
    crease_held_by[dst] = id;
    crease_held_by[src] = 0;
    batsman_wants[id]   = -1;
    pthread_cond_broadcast(&crease_cond);
    pthread_mutex_unlock(&rag_lock);
    GanttLogger::log(name, role, "RUN_END");
    return false;
}

void signal_played_bat(){
    pthread_mutex_lock(&umpire_lock);
    players_played_ball++;
    if(players_played_ball == NUM_FIELDERS + 2){
        std::cout << std::endl;
        pthread_cond_broadcast(&waiting_for_players_play_ball);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_result_of_ball_bat(){
    pthread_mutex_lock(&umpire_lock);
    while(!result_of_ball_ready){
        pthread_cond_wait(&waiting_for_result_of_ball, & umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
   
}

bool check_if_still_batsman(int id){
    pthread_mutex_lock(&pitch_lock);
    bool res = true;
    if(striker_id!=id && non_striker_id!=id){
        res = false;
    }
    pthread_mutex_unlock(&pitch_lock);
    return res;
}

void signal_result_of_ball_received_bat(){
    pthread_mutex_lock(&umpire_lock);
    players_received_result_of_ball++;
    if(players_received_result_of_ball == NUM_FIELDERS + 2){
        pthread_cond_signal(&waiting_for_players_to_receive_result_of_ball);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void* Batsman(void* arg){
    int id = *(int*)arg;

    wait_match_start_bat();
    wait_for_batsman_spot(id);
    while(!match_end){
        wait_for_next_ball_bat();
        pthread_mutex_lock(&pitch_lock);
bool is_striker = (striker_id == id);
pthread_mutex_unlock(&pitch_lock);

        if(is_striker){
            wait_ball_thrown();
            bat();
            attempt_run(id, STRIKER);
        }else{
            wait_ball_in_air_bat();
            attempt_run(id, NON_STRIKER);
        }
        signal_played_bat();
        wait_for_result_of_ball_bat();
        // do NOT use the attempt_run return value to break early.
        // The umpire may suppress a run-out (e.g. ball was also BOWLED), in
        // which case the victim must keep playing.  check_if_still_batsman()
        // is authoritative — it reflects what the umpire actually decided.
        bool chk = check_if_still_batsman(id) && !match_end;
        signal_result_of_ball_received_bat();

        if(!chk){
            break;
        }
    }
    // std::cout << "batsman:" << id << "exit\n";
    return NULL;
}