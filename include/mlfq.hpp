#ifndef SCHEDSIM_MLFQ_H
#define SCHEDSIM_MLFQ_H

#include <iostream>
#include <queue>
#include <array>
#include <atomic>
#include <type_traits>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <semaphore.h>
#include "types.hpp"
#include "task.hpp"

#define MLFQ_STOP_FLAG      0x1 // finish remaining tasks and stop
#define MLFQ_PRIO_FLAG      0x2 // priority boost 
#define MLFQ_STOP(flag)     ((flag) & MLFQ_STOP_FLAG)
#define MLFQ_PRIO(flag)     ((flag) & MLFQ_PRIO_FLAG)
#define MLFQ_EVENT(flag)    ((flag) & (0x3))
#define TIMESLICE_MS(level) (((level) + 1) * 20)    // 20, 40, ...
#define TIMESLICE_US(level) (((level) + 1) * 20000) // microsecond timeslice
#define PRIOBOOSTFREQ_MS    2500    // 2500 ms priority boost frequency
#define PRIOBOOSTFREQ_US    2500000 // priority boost frequency in microseconds

namespace scheduler {
class mlfq {
private:
    std::array<std::queue<task *>, 4>   tasks;      // array of queues per level
    pthread_mutex_t                     task_mtx;   // lock for task queue
    pthread_mutex_t                     io_mtx;     // lock for stdin/stdout
    sem_t                               sem;        // producer/consumer semaphore
    pthread_t                           *threads;   // workers
    u32                                 ncpus;      // number of cpus
    std::atomic<u8>                     flag;       // atomic flag for events
    
    u32 cpudiff(const struct rusage *prev, const struct rusage *cur) 
    const noexcept;
    void schedule(task *t, u32 lvl) noexcept;

    static void *schedworker(void *arg) noexcept;
    static void *prioboostworker(void *arg) noexcept;
public:
    /* default parameters: all processors, 4 queue levels */
    mlfq(u32 ncpus = get_nprocs()) noexcept;
    ~mlfq() noexcept; 

    void enqueue(task *t, u32 lvl = 0) noexcept; 
    
    /*  
     *  Heap allocate new task sub class constructed from argument list
     *  and push onto the task queue, acquiring mutex to protect task queue
     *  and increment semaphore for waiting scheduler threads
     */
    template<typename T, typename... Args>
    task *
    enqueue(Args &&...args) noexcept 
    requires std::is_constructible_v<T, Args...> && std::is_base_of_v<task, T>
    {
        task *t = new T(std::forward<Args>(args)...);
        pthread_mutex_lock(&task_mtx);
        tasks[0].push(t);
        sem_post(&sem);
        pthread_mutex_unlock(&task_mtx);
        return t;
    }
};


} // namespace scheduler
#endif
