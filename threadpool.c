#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include "threadpool.h"

#define QUEUE_SIZE 10
#define NUMBER_OF_THREADS 3

#define TRUE 1

// Define a task struct to hold function and data
typedef struct 
{
    void (*function)(void *p);
    void *data;
} task;

// Define the work queue as an array
task workqueue[QUEUE_SIZE];
int queue_size = 0;
int head = 0;
int tail = 0;

// Define mutex lock and semaphores
pthread_mutex_t queue_mutex;
sem_t full, empty;
pthread_t threads[NUMBER_OF_THREADS];
// Initialize the thread pool
void pool_init(void)
{
    pthread_mutex_init(&queue_mutex, NULL);
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, QUEUE_SIZE);

    
    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }
}

// Submit work to the pool
int pool_submit(void (*somefunction)(void *p), void *p)
{
    sem_wait(&empty);
    pthread_mutex_lock(&queue_mutex);

    if (queue_size < QUEUE_SIZE) {
        workqueue[tail].function = somefunction;
        workqueue[tail].data = p;
        tail = (tail + 1) % QUEUE_SIZE;
        queue_size++;

        pthread_mutex_unlock(&queue_mutex);
        sem_post(&full);

        return 0; // Success
    } else {
        pthread_mutex_unlock(&queue_mutex);
        sem_post(&empty);
        return 1; // Failure
    }
}

// Worker thread function
void *worker(void *param)
{
    while (TRUE) {
        sem_wait(&full);
        pthread_mutex_lock(&queue_mutex);

        if (queue_size > 0) {
            task current_task = workqueue[head];
            head = (head + 1) % QUEUE_SIZE;
            queue_size--;

            pthread_mutex_unlock(&queue_mutex);
            sem_post(&empty);

            // Execute the task
            execute(current_task.function, current_task.data);
        } else {
            pthread_mutex_unlock(&queue_mutex);
            sem_post(&full);
        }
    }
}

// Execute the task provided to the thread pool
void execute(void (*somefunction)(void *p), void *p)
{
    (*somefunction)(p);
}

void pool_shutdown(void)
{
    // Signal all worker threads to exit
    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        sem_post(&full);
    }

    // Wait for all worker threads to finish
    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Cleanup
    pthread_mutex_destroy(&queue_mutex);
    sem_destroy(&full);
    sem_destroy(&empty);
}

