#include <iostream>
#include "Bowler.h"
#include "shared.h"
#include "players.h"
#include "GanttLogger.h"

void wait_match_start_bowl(){
    pthread_mutex_lock(&umpire_lock);
    while(!match){
        pthread_cond_wait(&waiting_for_match, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_bowler_spot(int id){
    pthread_mutex_lock(&pitch_lock);
    // Wait if: spot taken, OR scheduler has designated a different bowler
    while(!match_end && (curr_bowler_id != 0 ||
          (scheduled_bowler_id != 0 && scheduled_bowler_id != id))){
        pthread_cond_wait(&waiting_for_bowler_spot, &pitch_lock);
    }
    if(match_end){
        pthread_mutex_unlock(&pitch_lock);
        return;
    }
    curr_bowler_id = id;
    scheduled_bowler_id = 0;   // clear so next over can be rescheduled
    new_bowler = true;
    pthread_cond_broadcast(&waiting_for_new_bowler);
    pthread_cond_broadcast(&waiting_for_bowler_chosen);
    pthread_mutex_unlock(&pitch_lock);
}

void wait_for_next_ball_bowl(){
    pthread_mutex_lock(&umpire_lock);
    while(!next_ball_ready){
        pthread_cond_wait(&waiting_for_next_ball, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void bowl(){
    std::string name = player_name(1 - current_team, curr_bowler_id);
    GanttLogger::log(name, "BOWLER", "BOWL_START");
    pthread_mutex_lock(&pitch_lock);
    ball_state = THROWN;
    pthread_cond_broadcast(&waiting_for_ball_thrown);
    pthread_mutex_unlock(&pitch_lock);
    GanttLogger::log(name, "BOWLER", "BOWL_END");
}

void signal_played_bowl(){
    pthread_mutex_lock(&umpire_lock);
    players_played_ball++;
    if(players_played_ball == NUM_FIELDERS + 2){
        std::cout << std::endl;
        pthread_cond_broadcast(&waiting_for_players_play_ball);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_result_of_ball_bowl(){
    pthread_mutex_lock(&umpire_lock);
    while(!result_of_ball_ready){
        pthread_cond_wait(&waiting_for_result_of_ball, & umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

bool check_if_still_bowler(int id){
    pthread_mutex_lock(&pitch_lock);
    bool res = true;
    if(curr_bowler_id!=id){
        res = false;
    }
    pthread_mutex_unlock(&pitch_lock);
    return res;
}

void signal_result_of_ball_received_bowl(){
    pthread_mutex_lock(&umpire_lock);
    players_received_result_of_ball++;
    if(players_received_result_of_ball == NUM_FIELDERS + 2){
        pthread_cond_signal(&waiting_for_players_to_receive_result_of_ball);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void* Bowler(void* arg){
    int id = *(int*)arg;
    wait_match_start_bowl();

    // Only bowlers and all-rounders are eligible to bowl
    int bowling_team = 1 - current_team;
    if (!can_bowl(bowling_team, id)) {
        // Not a bowler — sleep until match ends (woken by signal_sleeping_bowlers)
        pthread_mutex_lock(&pitch_lock);
        while (!match_end) {
            pthread_cond_wait(&waiting_for_bowler_spot, &pitch_lock);
        }
        pthread_mutex_unlock(&pitch_lock);
        return NULL;
    }

    while(!match_end){
        wait_for_bowler_spot(id);
        while(!match_end){
            wait_for_next_ball_bowl();
            bowl();
            // std::cout << "bowler_out_of_bowl\n";
            signal_played_bowl();
            wait_for_result_of_ball_bowl();
            // act on result
            bool chk = check_if_still_bowler(id);
            signal_result_of_ball_received_bowl();
            if(!chk)break;
        }
    }
    return NULL;
    // std::cout << "Bowler:" << id << "exit\n";
}