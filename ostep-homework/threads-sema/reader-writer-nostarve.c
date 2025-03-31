#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common_threads.h"
#include "semaphore.h"

//
// Your code goes in the structure and functions below
//

typedef struct __rwlock_t {
    sem_t r_lock;
    sem_t w_lock;
    sem_t reader_queue;
    int reader;
    int wait_reader;
    int wait_writer;
} rwlock_t;

void rwlock_init(rwlock_t *rw) {
    rw->reader = 0;
    rw->wait_reader = 0;
    rw->wait_writer = 0;

    sem_init(&rw->r_lock, 1);
    sem_init(&rw->w_lock, 1);
    sem_init(&rw->reader_queue, 0);
}

void rwlock_acquire_readlock(rwlock_t *rw) {
    if (rw->wait_writer) {
        sem_wait(&rw->r_lock);
        rw->wait_reader++;
        sem_post(&rw->r_lock);
        sem_wait(&rw->reader_queue);
    }
    sem_wait(&rw->r_lock);
    if (++rw->reader == 1) sem_wait(&rw->w_lock);
    sem_post(&rw->r_lock);
}

void rwlock_release_readlock(rwlock_t *rw) {
    sem_wait(&rw->r_lock);
    if (--rw->reader == 0) sem_post(&rw->w_lock);
    sem_post(&rw->r_lock);
}

void rwlock_acquire_writelock(rwlock_t *rw) {
    sem_wait(&rw->r_lock);
    rw->wait_writer++;
    sem_post(&rw->r_lock);

    sem_wait(&rw->w_lock);
}

void rwlock_release_writelock(rwlock_t *rw) {
    if (--rw->wait_writer == 0) {
        for (int i = 0; i < rw->wait_reader; i++)
            sem_post(&rw->reader_queue);

        rw->wait_reader = 0;
    }
    sem_post(&rw->w_lock);
}

//
// Don't change the code below (just use it!)
// 

int loops;
int value = 0;

rwlock_t lock;

void *reader(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
	rwlock_acquire_readlock(&lock);
	printf("read %d\n", value);
	rwlock_release_readlock(&lock);
    }
    return NULL;
}

void *writer(void *arg) {
    int i;
    for (i = 0; i < loops; i++) {
	rwlock_acquire_writelock(&lock);
	value++;
	printf("write %d\n", value);
	rwlock_release_writelock(&lock);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    assert(argc == 4);
    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);
    loops = atoi(argv[3]);

    pthread_t pr[num_readers], pw[num_writers];

    rwlock_init(&lock);

    printf("begin\n");

    int i;
    for (i = 0; i < num_readers; i++)
	Pthread_create(&pr[i], NULL, reader, NULL);
    for (i = 0; i < num_writers; i++)
	Pthread_create(&pw[i], NULL, writer, NULL);

    for (i = 0; i < num_readers; i++)
	Pthread_join(pr[i], NULL);
    for (i = 0; i < num_writers; i++)
	Pthread_join(pw[i], NULL);

    printf("end: value %d\n", value);

    return 0;
}

