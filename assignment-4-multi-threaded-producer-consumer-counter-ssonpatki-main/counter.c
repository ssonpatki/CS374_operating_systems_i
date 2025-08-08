// TODO Complete the assignment following the instructions in the assignment
// document. Refer to the Concurrency lecture notes in Canvas for examples.
// You may also find slides 50 and 51 from Ben Brewster's Concurrency slide
// deck to be helpful (you can find Brewster's slide decks in Canvas -> files).
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

struct thread_arg_t {
	int* my_count;
    int* message_printed;   // indicates is consumer message was printed
	pthread_mutex_t* my_mutex;
	pthread_cond_t* my_cond_1;  // signals consumer to wait for producer to be done
    pthread_cond_t* my_cond_2;  // signals producer to wait for consumer to be done
};

void* start_routine(void* arg) {
    struct thread_arg_t* casted_arg = arg;

    int previous = 0;

    while (1) {
        // Acquire the lock
        pthread_mutex_lock(casted_arg->my_mutex);
        printf("CONSUMER: MUTEX LOCKED\n");
        fflush(stdout);

        printf("CONSUMER: WAITING ON CONDITION VARIABLE 1\n");
        fflush(stdout);
        // wait for my_count to be incremented above previous value 
        while (*(casted_arg->my_count) <= previous) {
            pthread_cond_wait(casted_arg->my_cond_1, casted_arg->my_mutex);
        }
        
        printf("Count updated: %d -> %d\n", previous, *(casted_arg->my_count));
        fflush(stdout);

        previous = *(casted_arg->my_count);

        *(casted_arg->message_printed) = 1;
        printf("CONSUMER: SIGNALING CONDITION VARIABLE 2\n");
        fflush(stdout);
        pthread_cond_signal(casted_arg->my_cond_2);

        if (*(casted_arg->my_count) >= 10) {
             // release lock
            printf("CONSUMER: MUTEX UNLOCKED\n");
            fflush(stdout);
            pthread_mutex_unlock(casted_arg->my_mutex);
            break;
        }

        // release lock
        printf("CONSUMER: MUTEX UNLOCKED\n");
        fflush(stdout);
        pthread_mutex_unlock(casted_arg->my_mutex);
    }
    
    return NULL;
}

int main() {
    // main thread starts -- executes the main function
    printf("PROGRAM START\n");
    fflush(stdout);

    pthread_mutex_t my_mutex;
    pthread_mutex_init(&my_mutex, NULL);
    pthread_cond_t my_cond_1;
    pthread_cond_init(&my_cond_1, NULL);
    pthread_cond_t my_cond_2;
    pthread_cond_init(&my_cond_2, NULL);

    int my_count = 0;
    int message_printed = 0;

    pthread_t new_thread;
    struct thread_arg_t thread_arg;
    thread_arg.my_count = &my_count;
    thread_arg.my_mutex = &my_mutex;
    thread_arg.my_cond_1 = &my_cond_1;
    thread_arg.my_cond_2 = &my_cond_2;
    thread_arg.message_printed = &message_printed;

    // create a new thread and start it
    int create_success = pthread_create(&(new_thread), NULL, start_routine, &(thread_arg));

    if (create_success == 0) {
        printf("CONSUMER THREAD CREATED\n");
        fflush(stdout);
    } else {
        printf("Error when creating new thread\n");
    }

    while (my_count < 10) {
        pthread_mutex_lock(&my_mutex);
        printf("PRODUCER: MUTEX LOCKED\n");
        fflush(stdout);

        message_printed = 0;
        my_count++;
        
        printf("PRODUCER: SIGNALING CONDITION VARIABLE 1\n");
        fflush(stdout);
        pthread_cond_signal(&my_cond_1);

        
        printf("PRODUCER: WAITING ON CONDITION VARIABLE 2\n");
        fflush(stdout);
        while (message_printed == 0) {
            pthread_cond_wait(&my_cond_2, &my_mutex);
        }

        printf("PRODUCER: MUTEX UNLOCKED\n");
        fflush(stdout);
        pthread_mutex_unlock(&my_mutex);

    }

    void* my_variable;  // stores void pointer return value of the start
    // wait for new_thread, save return value & clean up resources
    pthread_join(new_thread, &my_variable);

    printf("PROGRAM END\n");

}
