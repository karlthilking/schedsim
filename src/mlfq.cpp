#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <iostream>
#include <queue>
#include <array>
#include <atomic>
#include <cassert>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/types.h>
#include <err.h>
#include "../include/types.hpp"
#include "../include/task.hpp"
#include "../include/mlfq.hpp"

namespace scheduler {
u32
mlfq::cpudiff(const struct rusage *cur, const struct rusage *prev) 
const noexcept
{
    return ((cur->ru_utime.tv_sec * 1000) +
            (cur->ru_utime.tv_usec / 1000) +
            (cur->ru_stime.tv_sec * 1000) +
            (cur->ru_stime.tv_usec / 1000)) -
           ((prev->ru_utime.tv_sec * 1000) +
            (prev->ru_utime.tv_usec / 1000) +
            (prev->ru_stime.tv_sec * 1000) +
            (prev->ru_stime.tv_usec / 1000));
}
 
void
mlfq::schedule(task *t, u32 lvl) noexcept
{
    const task_state state = t->get_state();
    const struct rusage *prev = t->get_rusage();
    struct rusage cur;
    
    switch (state) {
    /* task is running for the first time */
    case task_state::RUNNABLE:
        t->set_t_firstrun(high_resolution_clock::now());
        t->run();
        pthread_mutex_lock(&io_mtx);
        std::cout << *t << " started\n";
        pthread_mutex_unlock(&io_mtx);
        break;
    /* task was descheduled and now resuming */
    case task_state::STOPPED:
        /* 
         *  increment task waiting time by the difference between the current
         *  time and the time it was last stopped
         */
        t->increment_t_waiting(high_resolution_clock::now());
        kill(t->get_pid(), SIGCONT);
        break;
    default:
        err(EXIT_FAILURE, "unexpected task state");
    }
    t->set_state(task_state::RUNNING);
    
    /* let task run for its timeslice */
    usleep(TIMESLICE_US(lvl));
    kill(t->get_pid(), SIGSTOP);

    int rc, wstat;
    if ((rc = wait4(t->get_pid(), &wstat, WUNTRACED, &cur)) < 0)
        err(EXIT_FAILURE, "wait4");
    
    /* child process exited */
    if (WIFEXITED(wstat)) {
        assert(WEXITSTATUS(wstat) == 0);
        t->set_state(task_state::FINISHED);
        t->set_t_completion(high_resolution_clock::now());
        t->set_rusage(&cur);

        pthread_mutex_lock(&io_mtx);
        std::cout << *t << " exited\n";
        pthread_mutex_unlock(&io_mtx);
    } 
    /* child process was stopped from the stop signal */
    else if (WIFSTOPPED(wstat) && WSTOPSIG(wstat) == SIGSTOP) {
        t->set_state(task_state::STOPPED);
        t->set_t_laststop(high_resolution_clock::now());
        /* 
         *  If the process has accumulated more than a timeslice
         *  in cpu time at the queue level than demote it
         *  to a lower queue level
         */
        if (cpudiff(prev, &cur) >= TIMESLICE_MS(lvl)) {
            t->set_rusage(&cur);
            enqueue(t, (lvl < tasks.size() - 1) ? lvl + 1 : lvl);
        } else {
            /* 
             *  keep task on same queue level if it did not consume its
             *  full timeslice
             */
            enqueue(t, lvl);
        }
    }
}

void *
mlfq::prioboostworker(void *arg) noexcept
{
    mlfq *m = (mlfq *)arg;
    bool empty;
    while (1) {
        usleep(PRIOBOOSTFREQ_US);
        empty = true;
        pthread_mutex_lock(&(m->task_mtx));
        /* migrate all tasks to the top priority level */
        for (u32 lvl = 1; lvl < m->tasks.size(); ++lvl) {
            if (!m->tasks[lvl].empty()) {
                empty = false;
                while (!m->tasks[lvl].empty()) {
                    m->tasks[0].push(m->tasks[lvl].front());
                    m->tasks[lvl].pop();
                }
            }
        }
        pthread_mutex_unlock(&(m->task_mtx));
        if (empty && MLFQ_STOP(m->flag.load()))
            break;
    }
    return nullptr;
}

void *
mlfq::schedworker(void *arg) noexcept
{
    mlfq *m = (mlfq *)arg;
    task *t;
    u32 lvl;
    do {
        t = nullptr;
        sem_wait(&(m->sem));
        pthread_mutex_lock(&(m->task_mtx));
        for (lvl = 0; lvl < m->tasks.size(); ++lvl) {
            if (m->tasks[lvl].empty())
                continue;
            t = m->tasks[lvl].front();
            m->tasks[lvl].pop();
            break;
        }
        pthread_mutex_unlock(&(m->task_mtx));
        if (t)
            m->schedule(t, lvl);
        else
            break;
    } while (1);
    
    return nullptr;
}

mlfq::mlfq(u32 ncpus) noexcept
    : ncpus(ncpus),
      flag(0)
{
    pthread_mutex_init(&task_mtx, nullptr);
    pthread_mutex_init(&io_mtx, nullptr);
    sem_init(&sem, 0, 0);
    
    /* one scheduler thread per cpu + priority boost thread */
    threads = (pthread_t *)malloc(sizeof(pthread_t) * (ncpus + 1));
    cpu_set_t cpus;
    CPU_ZERO(&cpus);

    /* launch one scheduler thread per cpu and pin to that cpu */
    for (u32 i = 0; i < ncpus; ++i) {
        CPU_SET(i, &cpus);
        pthread_create(threads + i, nullptr, schedworker, this);
        if (pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpus) < 0)
            err(EXIT_FAILURE, "pthread_setaffinity_np");
        CPU_CLR(i, &cpus);
    }
    
    pthread_create(threads + ncpus, nullptr, prioboostworker, this);
}

/* join all threads and clean up all resources */
mlfq::~mlfq() noexcept
{
    flag.fetch_or(MLFQ_STOP_FLAG);
    for (u32 i = 0; i < ncpus; ++i)
        sem_post(&sem);
    for (u32 i = 0; i < ncpus; ++i)
        pthread_join(threads[i], nullptr);
    
    pthread_join(threads[ncpus], nullptr);
    pthread_mutex_destroy(&task_mtx);
    pthread_mutex_destroy(&io_mtx);
    sem_destroy(&sem);
    free(threads);
}

void
mlfq::enqueue(task *t, u32 lvl) noexcept
{
    pthread_mutex_lock(&task_mtx);
    tasks[lvl].push(t);
    sem_post(&sem);
    pthread_mutex_unlock(&task_mtx);
}
} // namespace scheduler
