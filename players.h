#pragma once
#include <string>

// Player ID mapping for IND vs PAK T20 2026
// Index i = player with id (i+1)

const std::string TEAM1_NAME = "India";
const std::string TEAM2_NAME = "Pakistan";

const std::string INDIA[11] = {
     "Rohit Sharma",        // Opener
     "Shubman Gill",        // Opener
     "Virat Kohli",         // No. 3
     "Suryakumar Yadav",    // No. 4
     "Rishabh Pant",        // No. 5 (WK)
     "Hardik Pandya",       // No. 6 (All-rounder)
     "Ravindra Jadeja",     // No. 7 (All-rounder)
     "Axar Patel",          // No. 8 (Bowling all-rounder)
     "Kuldeep Yadav",       // No. 9 (Spinner)
     "Mohammed Siraj",     // No. 10 (Pace)
     "Jasprit Bumrah",     // No. 11 (Death over specialist)
};

const std::string PAKISTAN[11] = {
     "Mohammad Rizwan",     // Opener (WK)
     "Babar Azam",          // Opener
     "Fakhar Zaman",        // No. 3
     "Iftikhar Ahmed",      // No. 4
     "Shadab Khan",         // No. 5 (All-rounder)
     "Azam Khan",           // No. 6
     "Imad Wasim",          // No. 7 (All-rounder)
     "Mohammad Wasim Jr.",  // No. 8 (Fast bowling all-rounder)
     "Shaheen Afridi",      // No. 9 (Pace)
     "Naseem Shah",        // No. 10 (Pace)
     "Haris Rauf",         // No. 11 (Death over specialist)
};

//get player name by team index (0=India, 1=Pakistan) and id (1-11)
inline std::string player_name(int team, int id) {
    if (id < 1 || id > 11) return "Unknown";
    return (team == 0 ? INDIA : PAKISTAN)[id - 1];
}

// India:Hardik(6),Jadeja(7),Axar(8),Kuldeep(9),Siraj(10),Bumrah(11)
// Pakistan:Shadab(5),Imad(7),Wasim Jr.(8),Shaheen(9),Naseem(10),Haris(11)
inline bool can_bowl(int team, int id) {
    if (team == 0) return id >= 6 && id <= 11;              // India
    else           return id == 5 || (id >= 7 && id <= 11); // Pakistan
}

// Death over specialist: Bumrah (India, id=11), Haris Rauf (Pakistan, id=11)
inline bool is_death_specialist(int team, int id) {
    return id == 11;   
}
