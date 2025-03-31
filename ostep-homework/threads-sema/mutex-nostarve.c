#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "common_threads.h"
#include "semaphore.h"

//
// Here, you have to write (almost) ALL the code. Oh no!
// How can you show that a thread does not starve
// when attempting to acquire this mutex you build?
//
#define QUEUE_LEN 16
#define LOOP_LEN 100

typedef struct {
    sem_t queue[QUEUE_LEN];
    sem_t lock;
    int queue_head_ptr;
    int queue_tail_ptr;
    int running_thread_num;
} ns_mutex_t;

ns_mutex_t mutex;

void ns_mutex_init(ns_mutex_t *m) {
    m->queue_head_ptr = 0;
    m->queue_tail_ptr = 0;
    m->running_thread_num = 0;
    sem_init(&m->lock, 1);
    for (int i = 0; i < QUEUE_LEN; i++)
        sem_init(&m->queue[i], 0);
}

void ns_mutex_acquire(ns_mutex_t *m) {
    sem_wait(&m->lock);
    m->running_thread_num++;
    sem_post(&m->lock);
    if (m->running_thread_num >= 2) {
        sem_wait(&m->lock);
        int sleep_ptr = m->queue_tail_ptr;
        m->queue_tail_ptr = (m->queue_tail_ptr + 1) % QUEUE_LEN;
        sem_post(&m->lock);
        sem_wait(&m->queue[sleep_ptr]);
    }
}

void ns_mutex_release(ns_mutex_t *m) {
    sem_wait(&m->lock);
    m->running_thread_num--;
    sem_post(&m->lock);
    if (m->running_thread_num >= 1) {
        sem_wait(&m->lock);
        int wake_ptr = m->queue_head_ptr;
        m->queue_head_ptr = (m->queue_head_ptr + 1) % QUEUE_LEN;
        sem_post(&m->lock);
        sem_post(&m->queue[wake_ptr]);
    }
}


void *worker(void *arg) {
    int tid = *((int *) arg);
    ns_mutex_acquire(&mutex);
    usleep(rand() % 1000);
    printf("tid: %d running\n", tid);
    ns_mutex_release(&mutex);
    return NULL;
}

int main(void) {
    pthread_t t[LOOP_LEN];
    int tids[LOOP_LEN];

    ns_mutex_init(&mutex); 

    printf("parent: begin\n");

    for (int i = 0; i < LOOP_LEN; i++) {
        tids[i] = i;
        pthread_create(&t[i], NULL, worker, &tids[i]);
    }

    for (int i = 0; i < LOOP_LEN; i++)
        pthread_join(t[i], NULL);

    printf("parent: end\n");
    return 0;
}

