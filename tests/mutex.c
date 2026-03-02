#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int
main(void)
{
    pthread_mutex_t mtx;
    sem_t sem;
    pthread_mutex_init(&mtx, NULL);
    sem_init(&sem, 0, 0);
    printf("sizeof(pthread_mutex_t): %zu\n", sizeof(pthread_mutex_t));
    printf("sizeof(sem_t): %zu\n", sizeof(sem_t));
    pthread_mutex_destroy(&mtx);
    sem_destroy(&sem);
    _exit(0);
}
