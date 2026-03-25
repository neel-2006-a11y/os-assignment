#include "Batsman.h"
#include "Bowler.h"
#include "Fielder.h"
#include "Umpire.h"
#include <pthread.h>

int main(){
    pthread_t fielder_threads[11];
    pthread_t bowler_threads[11];
    pthread_t batsman_threads[11];

    int ids[11];

    pthread_t umpire_thread;
    for (int i = 0; i < 11; i++)
    {
        ids[i] = i+1;
        pthread_create(&fielder_threads[i], NULL, Fielder, &ids[i]);
        pthread_create(&bowler_threads[i], NULL, Bowler, &ids[i]);
        pthread_create(&batsman_threads[i], NULL, Batsman, &ids[i]);
    }

    pthread_create(&umpire_thread, NULL, Umpire, NULL);

    for(int i = 0; i<11; i++){
        pthread_join(fielder_threads[i], NULL);
        pthread_join(bowler_threads[i], NULL);
        pthread_join(batsman_threads[i], NULL);
    }
    pthread_join(umpire_thread, NULL);
    return 0;
}