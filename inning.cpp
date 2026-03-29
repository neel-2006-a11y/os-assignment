#include "Batsman.h"
#include "Bowler.h"
#include "Fielder.h"
#include "Umpire.h"
#include "DeadlockDetector.h"
#include "GanttLogger.h"
#include "shared.h"
#include "players.h"
#include "scheduler.h"
#include <pthread.h>
#include <iostream>
#include <cstdlib>
#include <ctime>


void reset_state() {
    // Value resets
    match = false;
    match_end = false;

    fielders_received_bowler = 0;
    next_ball_ready = false;
    players_played_ball = 0;
    balls_played_in_over = 0;
    result_of_ball_ready = false;
    players_received_result_of_ball = 0;
    new_over = false;
    over_finished = 0;
    wickets = 0;
    total_runs = 0;
    last_event = "";
    curr_ball = 0;
    curr_over = 0;

    ball_state = INITIAL;

    prev_bowler_id = 0;
    curr_bowler_id = 0;
    new_bowler = false;
    scheduled_bowler_id = 0;

    striker_id = 0;
    non_striker_id = 0;
    batsmen_chosen = 0;
    next_bat_order = 1;
    striker_side_start_run = false;
    striker_side_end_run = false;
    non_striker_side_start_run = false;
    non_striker_side_end_run = false;

    catcher_id = 0;

    // Reinitialize synchronization primitives (all threads have exited by now)
    pthread_mutex_destroy(&pitch_lock);
    pthread_mutex_destroy(&umpire_lock);
    pthread_mutex_destroy(&print_lock);
    pthread_mutex_destroy(&rag_lock);
    pthread_mutex_init(&pitch_lock, NULL);
    pthread_mutex_init(&umpire_lock, NULL);
    pthread_mutex_init(&print_lock, NULL);
    pthread_mutex_init(&rag_lock, NULL);

    pthread_cond_destroy(&waiting_for_match);
    pthread_cond_destroy(&waiting_for_batsmen_chosen);
    pthread_cond_destroy(&waiting_for_bowler_chosen);
    pthread_cond_destroy(&waiting_for_new_bowler);
    pthread_cond_destroy(&waiting_for_fielders_received_bowler);
    pthread_cond_destroy(&waiting_for_next_ball);
    pthread_cond_destroy(&waiting_for_players_play_ball);
    pthread_cond_destroy(&waiting_for_result_of_ball);
    pthread_cond_destroy(&waiting_for_players_to_receive_result_of_ball);
    pthread_cond_destroy(&waiting_for_bowler_spot);
    pthread_cond_destroy(&waiting_for_ball_thrown);
    pthread_cond_destroy(&waiting_for_ball_hit);
    pthread_cond_destroy(&waiting_for_batsman_spot);
    pthread_cond_destroy(&crease_cond);

    pthread_cond_init(&waiting_for_match, NULL);
    pthread_cond_init(&waiting_for_batsmen_chosen, NULL);
    pthread_cond_init(&waiting_for_bowler_chosen, NULL);
    pthread_cond_init(&waiting_for_new_bowler, NULL);
    pthread_cond_init(&waiting_for_fielders_received_bowler, NULL);
    pthread_cond_init(&waiting_for_next_ball, NULL);
    pthread_cond_init(&waiting_for_players_play_ball, NULL);
    pthread_cond_init(&waiting_for_result_of_ball, NULL);
    pthread_cond_init(&waiting_for_players_to_receive_result_of_ball, NULL);
    pthread_cond_init(&waiting_for_bowler_spot, NULL);
    pthread_cond_init(&waiting_for_ball_thrown, NULL);
    pthread_cond_init(&waiting_for_ball_hit, NULL);
    pthread_cond_init(&waiting_for_batsman_spot, NULL);
    pthread_cond_init(&crease_cond, NULL);

    // Reset RAG state
    crease_held_by[0]      = 0;
    crease_held_by[1]      = 0;
    for(int i = 0; i <= 11; i++) batsman_wants[i] = -1;
    deadlock_detected      = false;
    deadlock_victim_id     = 0;
    run_collision_possible = false;
}

int run_inning(int inning_num, int ids[11]) {
    pthread_t fielder_threads[11];
    pthread_t bowler_threads[11];
    pthread_t batsman_threads[11];
    pthread_t umpire_thread;
    pthread_t deadlock_detector_thread;

    int batting_team = current_team;           // 0=India, 1=Pakistan
    int bowling_team = 1 - current_team;
    scheduler_init(bowling_team, batting_team);

    const std::string &team = (inning_num == 1) ? TEAM1_NAME : TEAM2_NAME;
    std::cout << "\n========== INNING " << inning_num << " : " << team << " batting ==========\n\n";

    for (int i = 0; i < 11; i++) {
        pthread_create(&fielder_threads[i], NULL, Fielder, &ids[i]);
        pthread_create(&bowler_threads[i], NULL, Bowler, &ids[i]);
        pthread_create(&batsman_threads[i], NULL, Batsman, &ids[i]);
    }
    pthread_create(&umpire_thread, NULL, Umpire, NULL);
    pthread_create(&deadlock_detector_thread, NULL, DeadlockDetector, NULL);

    for (int i = 0; i < 11; i++) {
        pthread_join(fielder_threads[i], NULL);
        pthread_join(bowler_threads[i], NULL);
        pthread_join(batsman_threads[i], NULL);
    }
    pthread_join(umpire_thread, NULL);
    pthread_join(deadlock_detector_thread, NULL);

    return total_runs;
}

int main() {
    srand(time(NULL));
    GanttLogger::init();  

    int ids[11];
    for (int i = 0; i < 11; i++) ids[i] = i + 1;

    // Inning 1: India bats
    current_team = 0;
    int score1 = run_inning(1, ids);
    team1_score = score1;
    int wickets1 = wickets;   // capture before reset clears it

    // Reset all state for inning 2
    reset_state();

    // Inning 2: Pakistan bats, chasing India's score
    current_team = 1;
    int score2 = run_inning(2, ids);
    team2_score = score2;

    // Capture state after inning 2 (before any reset)
    int wickets_fallen = wickets;                                          // wickets lost by chasing team
    int balls_used     = over_finished * 6 + balls_played_in_over;
    int balls_remaining = NUM_OVERS * 6 - balls_used;
    int wickets_remaining = 10 - wickets_fallen;

    // Final result
    std::cout << "\n========================================\n";
    std::cout << "             MATCH RESULT\n";
    std::cout << "          IND vs PAK T20 2026\n";
    std::cout << "========================================\n";
    std::cout << "  " << TEAM1_NAME << " (Inning 1): " << score1 << "/" << wickets1 << " runs\n";
    std::cout << "  " << TEAM2_NAME << " (Inning 2): " << score2 << "/" << wickets_fallen << " runs\n";
    std::cout << "----------------------------------------\n";

    if (score2 > score1) {
        // Chasing team won — wins by wickets (and balls remaining)
        std::cout << "  RESULT: " << TEAM2_NAME << " WINS by " << wickets_remaining
                  << " wicket(s) (" << balls_remaining << " ball(s) remaining)!\n";
    } else if (score1 > score2) {
        // Defending team won — wins by runs
        std::cout << "  RESULT: " << TEAM1_NAME << " WINS by " << (score1 - score2) << " run(s)!\n";
    } else {
        std::cout << "  RESULT: It's a TIE!\n";
    }
    std::cout << "========================================\n";

    GanttLogger::close();
    std::cout << "Logs are in gantt_log.txt";
    return 0;
}
