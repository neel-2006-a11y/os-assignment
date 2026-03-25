#include <iostream>
#include "Bowler.h"
#include "shared.h"

void wait_match_start_bowl(){
    pthread_mutex_lock(&umpire_lock);
    while(!match){
        pthread_cond_wait(&waiting_for_match, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_bowler_spot(int id){
    pthread_mutex_lock(&pitch_lock);
    while(curr_bowler_id!=0 && !match_end){
        pthread_cond_wait(&waiting_for_bowler_spot, &pitch_lock);
    }
    if(match_end){
        pthread_mutex_unlock(&pitch_lock);
        return;
    }
    curr_bowler_id = id;
    new_bowler = true;
    pthread_cond_broadcast(&waiting_for_new_bowler);
    std::cout << "bowler: " << id << std::endl;
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
    pthread_mutex_lock(&pitch_lock);
    ball_state = THROWN;
    pthread_cond_broadcast(&waiting_for_ball_thrown);
    pthread_mutex_unlock(&pitch_lock);
    std::cout << "ball thrown\n";
}

void signal_played_bowl(){
    pthread_mutex_lock(&umpire_lock);
    players_played_ball++;
    std::cout << "p::" << players_played_ball << " ";
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
    std::cout << "bowler_received_result\n";
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
    // std::cout << "Bowler:" << id << "exit\n";
}