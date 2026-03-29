#pragma once
#include "players.h"
#include <string>

// Per-bowler context saved on every Round-Robin context switch
struct BowlerCtx {
    int overs_bowled  = 0;
    int runs_given    = 0;
    int wickets_taken = 0;
};

extern BowlerCtx bowler_ctx[12];   // indexed by player ID 1-11

// Batting schedule: bat_order[position] = player_id
// Default 1-to-1; SJF reorders tail-enders (positions 9-11)
extern int bat_order[12];          // positions 1-11

// init 
// Call once per inning before threads start
void scheduler_init(int bowling_team, int batting_team);

// bowler scheduling
// Called after each over.  Saves outgoing bowler stats (RR context switch),
// then returns the next bowler ID using RR or Priority.
int scheduler_next_bowler(int bowling_team, int outgoing_id,
                          int runs_this_over, int wickets_this_over);

// helpers 
bool is_high_intensity();          // last over → Priority over round robin
bool is_tail_ender(int team, int id);  // IDs 9-11 are bowlers for batting
