#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

#define NCPUS 12

typedef struct {
    pthread_t   ptid;
    int         cpuid;
} arg_t;

void *
routine(void *__arg)
{
    arg_t *arg = (arg_t *)__arg;
    cpu_set_t cpu;
    CPU_ZERO(&cpu);
    CPU_SET(arg->cpuid, &cpu);
    pthread_setaffinity_np(arg->ptid, sizeof(cpu_set_t), &cpu);

    if (CPU_ISSET(arg->cpuid, &cpu))
        printf("Running on cpu %d\n", arg->cpuid);
    else
        printf("Failed\n");
        
    return NULL;
}

int
main(void)
{
    arg_t args[NCPUS];
    for (int i = 0; i < NCPUS; ++i) {
        args[i].cpuid = i;
        pthread_create(&(args[i].ptid), NULL, routine, args + i);
    }
    for (int i = 0; i < NCPUS; ++i)
        pthread_join(args[i].ptid, NULL);
    _exit(0);
}
