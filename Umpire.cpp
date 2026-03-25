#include <pthread.h>

#include "Umpire.h"
#include "shared.h"

void match_start(){
    pthread_mutex_lock(&umpire_lock);
    match = true;
    pthread_cond_broadcast(&waiting_for_match);
    pthread_mutex_unlock(&umpire_lock);
}

void wait_two_batsmen(){
    pthread_mutex_lock(&pitch_lock);
    while(batsmen_chosen<2){
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

void wait_for_fielders_to_receive_new_bowler(){
    pthread_mutex_lock(&umpire_lock);
    while(fielders_received_bowler<NUM_FIELDERS){
        pthread_cond_wait(&waiting_for_fielders_received_bowler, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void set_next_ball_ready(){
    pthread_mutex_lock(&umpire_lock);
    next_ball_ready = true;
    pthread_cond_broadcast(&waiting_for_next_ball);
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_all_players_to_play_ball(){
    pthread_mutex_lock(&umpire_lock);
    while(players_played_ball<NUM_FIELDERS + 2){
        pthread_cond_wait(&waiting_for_players_play_ball, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void get_new_bowler(){
    pthread_mutex_lock(&pitch_lock);
    prev_bowler_id = curr_bowler_id;
    curr_bowler_id = 0;
    pthread_cond_broadcast(&waiting_for_bowler_spot);
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
    pthread_mutex_unlock(&umpire_lock);
}

void* Umpire(void* arg){
    match_start();
    
    while(true){
        wait_bowler_chosen();
        while(true){
            wait_two_batsmen();

            if(new_bowler){
                pthread_cond_broadcast(&waiting_for_new_bowler);
                wait_for_fielders_to_receive_new_bowler();
                new_bowler = false;
            }

            // reset list
            fielders_received_bowler = 0;

            set_next_ball_ready();

            wait_for_all_players_to_play_ball();

            // no player accessing next_ball, all next waiting for result
            next_ball_ready = false;
            players_played_ball = 0;

            // make result
            if(balls_played_in_over == 6){
                balls_played_in_over = 0;
                get_new_bowler();
                new_over = true;
            }

            // signal result of ball
            signal_result_of_ball();
            wait_for_players_to_receive_result_of_ball();

            // reset list
            players_received_result_of_ball = 0;
            result_of_ball_ready = false;
            if(new_over){
                new_over = false;
                break;
            }
        }
    }
}