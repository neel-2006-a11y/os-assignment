#include <pthread.h>
#include <chrono>
#include <iostream>
#include "Batsman.h"
#include "shared.h"

void wait_match_start_bat(){
    pthread_mutex_lock(&umpire_lock);
    while(!match){
        pthread_cond_wait(&waiting_for_match, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_batsman_spot(int id){
    pthread_mutex_lock(&pitch_lock);
    while(striker_id!=0 && non_striker_id!=0 && !match_end){
        pthread_cond_wait(&waiting_for_batsman_spot,&pitch_lock);
    }
    if(striker_id == 0){
        striker_id = id;
        std::cout << "striker: " << id << "\n";
    }else{
        non_striker_id = id;
        std::cout << "non_striker: " << id << "\n";
    }
    batsmen_chosen++;
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
    pthread_mutex_lock(&pitch_lock);
    ball_state = IN_AIR;
    std::cout << "ball_in_air\n";
    pthread_cond_broadcast(&waiting_for_ball_hit);
    pthread_mutex_unlock(&pitch_lock);
}

void run_start(PitchSide side){
    pthread_mutex_lock(&pitch_lock);
    if(side == STRIKER){
        std::cout << "striker_run_start\n";
        striker_side_start_run = true;
    }else{
        std::cout << "non_striker_run_start\n";
        non_striker_side_start_run = true;
    }
    pthread_mutex_unlock(&pitch_lock);
}

void run_end(PitchSide side){
    pthread_mutex_lock(&pitch_lock);
    if(side == STRIKER){
        std::cout << "striker_run_end\n";
        striker_side_end_run = true;
        striker_side_reach_time = std::chrono::steady_clock::now();
    }else{
        std::cout << "non_striker_run_end\n";
        non_striker_side_end_run = true;
        non_striker_side_reach_time = std::chrono::steady_clock::now();
    }
    pthread_mutex_unlock(&pitch_lock);
}

void signal_played_bat(){
    pthread_mutex_lock(&umpire_lock);
    players_played_ball++;
    std::cout << "p::" << players_played_ball << " ";
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
        if(striker_id == id){
            wait_ball_thrown();
            bat();
            run_start(STRIKER);
            // delay
            run_end(NON_STRIKER);
        }else{
            wait_ball_in_air_bat();
            std::cout << "non_striker_ball_in_air\n";
            run_start(NON_STRIKER);
            // delay
            run_end(STRIKER);
        }
        signal_played_bat();
        wait_for_result_of_ball_bat();
        bool chk = check_if_still_batsman(id);
        chk = chk && !match_end;
        signal_result_of_ball_received_bat();

        if(!chk){
            break;
        }
    }
    // std::cout << "batsman:" << id << "exit\n";
}