#pragma once
#include <string>
// Thread-safe: any player thread may call log() concurrently.
// Events: BOWL_START BOWL_END  BAT_START BAT_END
//         RUN_START  RUN_END   CATCH     DEADLOCK_RUNOUT
namespace GanttLogger {
    void init(); 
    void log(const std::string& player,
             const std::string& role,
             const std::string& event);
    void close();   
}
