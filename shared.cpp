#include "shared.h"

// Mutex
pthread_mutex_t pitch_lock = PTHREAD_MUTEX_INITIALIZER;

// Condition variables
pthread_cond_t waiting_for_ball = PTHREAD_COND_INITIALIZER;

// Shared state
