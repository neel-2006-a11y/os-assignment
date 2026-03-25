#include <pthread.h>
#include "Fielder.h"
#include "shared.h"

void wait_match_start(){
    pthread_mutex_lock(&umpire_lock);
    while(!match){
        pthread_cond_wait(&waiting_for_match, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_new_bowler(){
    pthread_mutex_lock(&pitch_lock);
    while(!new_bowler){
        pthread_cond_wait(&waiting_for_new_bowler, &pitch_lock);
    }
    pthread_mutex_unlock(&pitch_lock);
}

void signal_new_bowler_received(){
    pthread_mutex_lock(&umpire_lock);
    fielders_received_bowler++;
    if(fielders_received_bowler == NUM_FIELDERS){
        pthread_cond_broadcast(&waiting_for_fielders_received_bowler);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_next_ball(){
    pthread_mutex_lock(&umpire_lock);
    while(!next_ball_ready){
        pthread_cond_wait(&waiting_for_next_ball, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void field();

void signal_played(){
    pthread_mutex_lock(&umpire_lock);
    players_played_ball++;
    if(players_played_ball == NUM_FIELDERS + 2){
        pthread_cond_broadcast(&waiting_for_players_play_ball);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_result_of_ball(){
    pthread_mutex_lock(&umpire_lock);
    while(!result_of_ball_ready){
        pthread_cond_wait(&waiting_for_result_of_ball, & umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void signal_result_of_ball_received(){
    pthread_mutex_lock(&umpire_lock);
    players_received_result_of_ball++;
    if(players_received_result_of_ball == NUM_FIELDERS + 2){
        pthread_cond_signal(&waiting_for_players_to_receive_result_of_ball);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void* Fielder(void* arg){
    int id = *(int*)arg;
    wait_match_start();
    while(true){
        wait_new_bowler();
        while(true){
            if(curr_bowler_id == id){
                signal_new_bowler_received();
                break;
            }
            signal_new_bowler_received();

            wait_for_next_ball();

            field();

            signal_played();
            wait_for_result_of_ball();
            if(new_over){
                signal_result_of_ball_received();
                break;
            }
            signal_result_of_ball_received();
        }
        
    }
}