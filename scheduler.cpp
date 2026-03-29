#include "scheduler.h"
#include "shared.h"
#include <iostream>
#include <vector>

//storage
BowlerCtx bowler_ctx[12];
int       bat_order[12];   // bat_order[pos] = player_id

static std::vector<int> bowl_rotation;   // eligible bowler IDs in RR order
static int rr_idx = 0;

//helpers
bool is_high_intensity() {
    // Priority scheduling only during a run chase (inning 2) from the 19th over onward.
    // Assumption: death specialist matters most when defending a target under pressure;
    // over_finished >= NUM_OVERS-2 means 18 overs are done → 19th over is now being bowled.
    return current_team == 1 && over_finished >= NUM_OVERS - 2;
}

bool is_tail_ender(int /*team*/, int id) {
    return id >= 9;   // IDs 9-11: specialist bowlers batting at the tail
}

// ── scheduler_init ────────────────────────────────────────────────────────────
void scheduler_init(int bowling_team, int batting_team) {
    // Reset bowler contexts
    for (int i = 0; i <= 11; i++) bowler_ctx[i] = {};

    // Build RR rotation from eligible bowlers (preserves squad order)
    bowl_rotation.clear();
    rr_idx = 0;
    for (int id = 1; id <= 11; id++)
        if (can_bowl(bowling_team, id))
            bowl_rotation.push_back(id);

    // Build batting order based on scheduler toggle
    // FCFS (false): ascending  1 - 2 - ......- 11  (openers first, natural order)
    // SJF  (true) : descending 11 - 10 - .....- 1 (bowlers/worst batters first)
    for (int i = 1; i <= 11; i++)
        bat_order[i] = sjf_scheduler ? (12 - i) : i;

    std::cout << "\n[Scheduler] ── Inning setup ──────────────────────────────\n"
              << "[Scheduler] Bowling team : " << (bowling_team==0 ? TEAM1_NAME : TEAM2_NAME) << "\n"
              << "[Scheduler] Algorithm : Round-Robin (overs) | "
              << (sjf_scheduler ? "SJF" : "FCFS") << " | Priority (last over, inning 2)\n"
              << "[Scheduler] Batting order:\n";
    std::string mode_tag = sjf_scheduler ? "  [SJF]" : "  [FCFS]";
    for (int i = 1; i <= 11; i++) {
        std::cout << "[Scheduler]   " << i << ". "
                  << player_name(batting_team, bat_order[i]) << mode_tag << "\n";
    }
    std::cout << "[SCHEDULER] ──────────────────────────────────────────────\n\n";
}

int scheduler_next_bowler(int bowling_team, int outgoing_id,
                          int runs_this_over, int wickets_this_over) {
    // After RR context switch(save outgoing bowler stats)
    if (outgoing_id >= 1 && outgoing_id <= 11) {
        bowler_ctx[outgoing_id].overs_bowled++;
        bowler_ctx[outgoing_id].runs_given    += runs_this_over;
        bowler_ctx[outgoing_id].wickets_taken += wickets_this_over;

        std::cout << "[SCHEDULER] Context-switch OUT → "
                  << player_name(bowling_team, outgoing_id)
                  << " | Total runs given: "
                  << bowler_ctx[outgoing_id].runs_given    << " | "
                  << " | Total wickets taken: "
                  << bowler_ctx[outgoing_id].wickets_taken << "W in "
                  << bowler_ctx[outgoing_id].overs_bowled  << " over(s)\n";
    }

    if (bowl_rotation.empty()) return 0;

    //PRIORITY SCHEDULING: (last over is given to death over specialist)
    if (is_high_intensity()) {
        for (int id : bowl_rotation) {
            if (is_death_specialist(bowling_team, id)) {
                std::cout << "[SCHEDULER] PRIORITY (last over) → "
                          << player_name(bowling_team, id) << " gets the ball\n";
                return id;
            }
        }
    }

    //ROUND-ROBIN:(skip the outgoing bowler)
    int n = (int)bowl_rotation.size();
    int tries = 0;
    do {
        rr_idx = (rr_idx + 1) % n;
        tries++;
    } while (bowl_rotation[rr_idx] == outgoing_id && tries < n);

    int next_id = bowl_rotation[rr_idx];
    std::cout << "[SCHEDULER] RR next bowler → "
              << player_name(bowling_team, next_id) << "\n";
    return next_id;
}
