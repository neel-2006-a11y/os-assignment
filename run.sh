#!/bin/bash

# compile cricket simulator
g++ -o cricket inning.cpp Batsman.cpp Bowler.cpp Fielder.cpp Umpire.cpp shared.cpp scheduler.cpp DeadlockDetector.cpp GanttLogger.cpp -lpthread 2>&1 && ./cricket 2>&1