#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <pthread.h>

typedef struct {
    int value;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} sem_t;

void sem_init(sem_t *s, int value);
void sem_wait(sem_t *s);
void sem_post(sem_t *s);

#endif // _SEMAPHORE_H_
