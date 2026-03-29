#include "GanttLogger.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <pthread.h>

namespace GanttLogger {

static std::ofstream          out;
static std::chrono::steady_clock::time_point t0;
static pthread_mutex_t        mtx = PTHREAD_MUTEX_INITIALIZER;

void init() {
    out.open("gantt_log.txt", std::ios::trunc);
    out << "============================================================\n"
        << "   PITCH RESOURCE GANTT LOG — Thread activity over time\n"
        << "============================================================\n"
        << std::left
        << std::setw(14) << "Time (us)"
        << std::setw(14) << "Role"
        << std::setw(26) << "Player"
        << "Event\n"
        << "------------------------------------------------------------\n";
    t0 = std::chrono::steady_clock::now();
}

void log(const std::string& player,
         const std::string& role,
         const std::string& event) {
    long long us = std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::steady_clock::now() - t0).count();
    pthread_mutex_lock(&mtx);
    out << std::left
        << std::setw(14) << us
        << std::setw(14) << role
        << std::setw(26) << player
        << event << "\n";
    pthread_mutex_unlock(&mtx);
}

void close() {
    if (out.is_open()) {
        out << "============================================================\n";
        out.flush();
        out.close();
    }
}

} // namespace GanttLogger