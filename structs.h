#pragma once

enum BallState{
    INITIAL,
    THROWN,
    IN_AIR,
    CAUGHT
};

enum PitchSide{
    STRIKER,
    NON_STRIKER
};

struct Ball{
    int id;
};