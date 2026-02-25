#ifndef SCHEDSIM_MLFQ_H
#define SCHEDSIM_MLFQ_H

#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include "types.hpp"
#include "task.hpp"

#define MLFQ_STOP_FLAG      0x1 // finish remaining tasks and stop
#define MLFQ_HALT_FLAG      0x2 // stop immediately 
#define MLFQ_PRIO_FLAG      0x4 // priority boost 
#define MLFQ_STOP(flag)     ((flag) & MLFQ_STOP_FLAG)
#define MLFQ_HALT(flag)     ((flag) & MLFQ_HALT_FLAG)
#define MLFQ_PRIO(flag)     ((flag) & MLFQ_PRIO_FLAG)
#define TIMESLICE(level)    milliseconds((((level) + 1) * 20))
#define PRIO_BOOST_FREQ     milliseconds(2500)

namespace scheduler {
class mlfq {
private:
    std::vector<std::queue<task *>>         tasks;
    std::vector<std::thread>                threads;
    std::vector<std::condition_variable>    conds;
    std::vector<std::mutex>                 locks;
    std::mutex                              io_mutex;
    std::atomic<u8>                         flag;
    
    u32
    cpu_diff(const struct rusage &prev, const struct rusage &cur) 
    const noexcept;
    
    bool waiting_tasks() const noexcept;
    void schedule(task *t, u32 lvl) noexcept;
public:
    mlfq(u32 ncpus, u32 nlevels) noexcept;
    ~mlfq() noexcept; 
    void halt() noexcept; 

    void enqueue(task *t, u32 lvl = 0) noexcept; 

    template<typename T>
    void
    enqueue(T &t, u32 lvl = 0) noexcept
    {
        std::lock_guard<std::mutex> lock(locks[lvl]);
        tasks[lvl].push(new task(t));
        conds[lvl].notify_one();
    }
    
    template<typename T, typename ...Args>
    void
    enqueue(Args &&...args) noexcept
    {
        std::lock_guard<std::mutex> lock(locks[0]);
        tasks[0].push(new task(std::forward<Args>(args)...));
        conds[0].notify_one();
    }

    template<typename It>
    void
    enqueue(It begin, It end, u32 lvl = 0)
    {
        std::lock_guard<std::mutex> lk(locks[lvl]);
        for (auto it = begin; it != end; ++it)
            tasks[lvl].push(*it);
        conds[lvl].notify_all();
    }
};
} // namespace scheduler
#endif
