#include "DeadlockDetector.h"
#include "shared.h"
#include "players.h"
#include "GanttLogger.h"
#include <unistd.h>
#include <iostream>
#include <cstdlib>

// Polls the Resource Allocation Graph every 50 µs.
// A cycle exists when:
//   batsman A holds crease[0] and wants crease[1]
//   batsman B holds crease[1] and wants crease[0]
// → Both are blocked waiting for each other → circular wait → DEADLOCK.
// Resolution: randomly pick one as the victim (Run Out), wake both.

void* DeadlockDetector(void* arg) {
    while (!match_end) {
        usleep(50);   // poll every 50 microseconds

        pthread_mutex_lock(&rag_lock);

        // Skip: deadlock already handled, or no collision scenario this ball,
        // or one of the creases is still unoccupied (batsmen not yet at ends)
        if (deadlock_detected || !run_collision_possible
                || crease_held_by[0] == 0 || crease_held_by[1] == 0) {
            pthread_mutex_unlock(&rag_lock);
            continue;
        }

        int a = crease_held_by[0];   // batsman at striker end
        int b = crease_held_by[1];   // batsman at non-striker end

        // Cycle check: A wants B's crease AND B wants A's crease
        if (batsman_wants[a] == 1 && batsman_wants[b] == 0) {
            deadlock_detected   = true;
            deadlock_victim_id  = (rand() % 2 == 0) ? a : b;

            std::string victim_name = player_name(current_team, deadlock_victim_id);
            std::cout << "\n[DEADLOCK] Circular wait detected!\n"
                      << "[DEADLOCK]   " << player_name(current_team, a)
                      << " holds End-1, wants End-2\n"
                      << "[DEADLOCK]   " << player_name(current_team, b)
                      << " holds End-2, wants End-1\n"
                      << "[DEADLOCK] Umpire (kernel) kills → "
                      << victim_name << " is RUN OUT\n";

            GanttLogger::log(victim_name, "DEADLOCK", "RUNOUT");
            pthread_cond_broadcast(&crease_cond);   // wake both blocked batsmen
        }

        pthread_mutex_unlock(&rag_lock);
    }
    return NULL;
}
