#include "Bowler.h"
#include "shared.h"

void wait_match_start(){
    pthread_mutex_lock(&umpire_lock);
    while(!match){
        pthread_cond_wait(&waiting_for_match, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_bowler_spot(int id){
    pthread_mutex_lock(&pitch_lock);
    while(curr_bowler_id!=0){
        pthread_cond_wait(&waiting_for_bowler_spot, &pitch_lock);
    }
    curr_bowler_id = id;
    new_bowler = true;
    pthread_cond_broadcast(&waiting_for_bowler_chosen);
    pthread_mutex_unlock(&pitch_lock);
}

void wait_for_next_ball(){
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
}

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

bool check_if_still_bowler(int id){
    pthread_mutex_lock(&pitch_lock);
    bool res = true;
    if(curr_bowler_id!=id){
        res = false;
    }
    pthread_mutex_unlock(&pitch_lock);
    return res;
}

void signal_result_of_ball_received(){
    pthread_mutex_lock(&umpire_lock);
    players_received_result_of_ball++;
    if(players_received_result_of_ball == NUM_FIELDERS + 2){
        pthread_cond_signal(&waiting_for_players_to_receive_result_of_ball);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void* Bowler(void* arg){
    int id = *(int*)arg;
    wait_match_start();
    while(true){
        wait_for_bowler_spot(id);
        while(true){
            wait_for_next_ball();
            bowl();
            signal_played();
            wait_for_result_of_ball();
            // act on result
            bool chk = check_if_still_bowler(id);
            signal_result_of_ball_received();
            if(!chk)break;
        }
        // end match logic
    }
}