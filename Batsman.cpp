#include <pthread.h>
#include "Batsman.h"
#include "shared.h"

void wait_match_start(){
    pthread_mutex_lock(&umpire_lock);
    while(!match){
        pthread_cond_wait(&waiting_for_match, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void wait_for_batsman_spot(int id){
    pthread_mutex_lock(&pitch_lock);
    while(striker_id!=0 && non_striker_id!=0){
        pthread_cond_wait(&waiting_for_batsman_spot,&pitch_lock);
    }
    if(striker_id == 0){
        striker_id = id;
    }else{
        non_striker_id = id;
    }
    batsmen_chosen++;
    if(batsmen_chosen == 2){
        pthread_cond_broadcast(&waiting_for_batsmen_chosen);
    }
    pthread_mutex_unlock(&pitch_lock);
}

void wait_for_next_ball(){
    pthread_mutex_lock(&umpire_lock);
    while(!next_ball_ready){
        pthread_cond_wait(&waiting_for_next_ball, &umpire_lock);
    }
    pthread_mutex_unlock(&umpire_lock);
}

void bat(){

}
void run(PitchSide side){
    
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

bool check_if_still_batsman(int id){
    pthread_mutex_lock(&pitch_lock);
    bool res = true;
    if(striker_id!=id && non_striker_id!=id){
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

void* Batsman(void* arg){
    int id = *(int*)arg;

    wait_match_start();
    wait_for_batsman_spot(id);
    while(true){
        wait_for_next_ball();
        if(striker_id == id){
            bat();
            run(NON_STRIKER);
        }else{
            run(STRIKER);
        }
        signal_played();
        wait_for_result_of_ball();
        bool chk = check_if_still_batsman(id);
        signal_result_of_ball_received();

        if(!chk){
            break;
        }
    }
}