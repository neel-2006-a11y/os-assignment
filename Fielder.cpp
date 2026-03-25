#include <pthread.h>
#include <chrono>
#include <stdlib.h>
#include <iostream>
#include "Fielder.h"
#include "shared.h"

void wait_match_start_field(int id){
    pthread_mutex_lock(&umpire_lock);
    while(!match){
        pthread_cond_wait(&waiting_for_match, &umpire_lock);
    }
    // std::cout << "fielder_id: " << id << std::endl;
    pthread_mutex_unlock(&umpire_lock);
}

void wait_new_bowler_field(){
    pthread_mutex_lock(&pitch_lock);
    while(!new_bowler && !match_end){
        pthread_cond_wait(&waiting_for_new_bowler, &pitch_lock);
    }
    pthread_mutex_unlock(&pitch_lock);
}

void signal_new_bowler_received(){
    pthread_mutex_lock(&umpire_lock);
    fielders_received_bowler++;
    std::cout << "f::" << fielders_received_bowler << " ";
    if(fielders_received_bowler == NUM_FIELDERS){
        std::cout << std::endl;
        pthread_cond_broadcast(&waiting_for_fielders_received_bowler);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_next_ball_field(){
    pthread_mutex_lock(&umpire_lock);
    while(!next_ball_ready){
        pthread_cond_wait(&waiting_for_next_ball, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_ball_in_air_field(){
    pthread_mutex_lock(&pitch_lock);
    while(ball_state!=IN_AIR){
        pthread_cond_wait(&waiting_for_ball_hit, &pitch_lock);
    }
    pthread_mutex_unlock(&pitch_lock);
}

void field(int id){
    pthread_mutex_lock(&pitch_lock);

    if(catcher_id == 0){
        int bit = rand()%2;
        if(bit){
            throw_side = STRIKER;
        }else{
            throw_side = NON_STRIKER;
        }
        catcher_id = id;
        fielder_ball_reach_time = std::chrono::steady_clock::now();
    }
    pthread_mutex_unlock(&pitch_lock);
}

void signal_played_field(){
    pthread_mutex_lock(&umpire_lock);
    players_played_ball++;
    std::cout << "p::" << players_played_ball << " ";
    if(players_played_ball == NUM_FIELDERS + 2){
        std::cout << std::endl;
        pthread_cond_broadcast(&waiting_for_players_play_ball);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_result_of_ball_field(){
    pthread_mutex_lock(&umpire_lock);
    while(!result_of_ball_ready){
        pthread_cond_wait(&waiting_for_result_of_ball, & umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void signal_result_of_ball_received_field(){
    pthread_mutex_lock(&umpire_lock);
    players_received_result_of_ball++;
    if(players_received_result_of_ball == NUM_FIELDERS + 2){
        pthread_cond_signal(&waiting_for_players_to_receive_result_of_ball);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void* Fielder(void* arg){
    int id = *(int*)arg;
    wait_match_start_field(id);
    while(!match_end){
        wait_new_bowler_field();
        if(match_end)break;
        if(curr_bowler_id == id){
            signal_new_bowler_received();
            wait_for_next_ball_field();
            continue;
        }else{
            signal_new_bowler_received();
        }
        while(true){
            wait_for_next_ball_field();
            wait_ball_in_air_field();
            // delay
            field(id);

            signal_played_field();
            wait_for_result_of_ball_field();
            if(new_over || match_end){
                signal_result_of_ball_received_field();
                break;
            }
            signal_result_of_ball_received_field();
        }

    }
    // std::cout << "Fielder:" << id << "exit\n";
}