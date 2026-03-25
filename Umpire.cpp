#include <pthread.h>
#include <iostream>

#include "Umpire.h"
#include "shared.h"

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
    std::cout << "fielders_received_new_bowler\n";
    pthread_mutex_unlock(&umpire_lock);
}

void set_next_ball_ready(){
    pthread_mutex_lock(&umpire_lock);
    next_ball_ready = true;
    ball_state = INITIAL;
    std::cout << "next_ball_set\n";
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
    catcher_id = 0;
    
    pthread_mutex_unlock(&umpire_lock);
}

void get_new_bowler(){
    pthread_mutex_lock(&pitch_lock);
    prev_bowler_id = curr_bowler_id;
    curr_bowler_id = 0;
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
}

void* Umpire(void* arg){
    match_start();
    // std::cout << "match started\n";
    
    while(!match_end){
        wait_new_bowler_umpire();
        // std::cout << "bowler_chosen id: " << curr_bowler_id << std::endl;
        while(true){
            wait_two_batsmen();
            std::cout << "/////////////\n";
            std::cout << "batsman_chosen\n";

            if(new_bowler){
                // signal_new_bowler();
                wait_for_fielders_to_receive_new_bowler();
            }

            set_next_ball_ready();

            wait_for_all_players_to_play_ball();

            // no player accessing next_ball, all next waiting for result

            // make result
            // run_out
            if(throw_side == STRIKER){
                if(striker_side_reach_time > fielder_ball_reach_time){
                    batsmen_chosen--;
                    get_new_batsman(STRIKER);
                    wickets++;
                }
            }else{
                if(non_striker_side_reach_time > fielder_ball_reach_time){
                    batsmen_chosen--;
                    get_new_batsman(NON_STRIKER);
                    wickets++;
                }
            }

            // change over
            if(balls_played_in_over == 6){
                balls_played_in_over = 0;
                get_new_bowler();
                new_over = true;
                over_finished++;
            }

            if(wickets==10 || over_finished == NUM_OVERS){
                match_end = true;
                signal_sleeping_fielder();
                signal_sleeping_bowlers();
                signal_sleeping_batsman();
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

    std::cout << "umpire exit\n";
}