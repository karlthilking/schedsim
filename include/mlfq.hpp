#ifndef SCHEDSIM_MLFQ_H
#define SCHEDSIM_MLFQ_H

#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <type_traits>
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
#define MLFQ_EVENT(flag)    ((flag) & 0x7)
#define TIMESLICE(level)    milliseconds((((level) + 1) * 20))
#define PRIO_BOOST_FREQ     milliseconds(2500)

namespace scheduler {
class mlfq {
private:
    std::vector<std::queue<task *>>         tasks;      // 24 bytes
    std::vector<std::thread>                threads;    // 24 bytes
    std::vector<std::condition_variable>    conds;      // 24 bytes
    std::vector<std::mutex>                 locks;      // 24 bytes
    std::mutex                              io_mutex;   // 40 bytes
    std::atomic<u8>                         flag;       // 1 byte
    
    u32
    cpu_diff(const struct rusage &prev, const struct rusage &cur) 
    const noexcept;
    
    bool waiting_tasks() const noexcept;
    void schedule(task *t, u32 lvl) noexcept;
public:
    mlfq(u32 ncpus, u32 nlevels) noexcept;
    ~mlfq() noexcept; 
    // void halt() noexcept; 

    void enqueue(task *t, u32 lvl = 0) noexcept; 
    
    /* return pointer so caller can manage object lifetime */
    template<typename T>
    task * 
    enqueue(T &&t, u32 lvl = 0) noexcept
    requires std::is_base_of_v<task, T>
    {
        task *tp = new T(std::forward<T>(t));
        std::lock_guard<std::mutex> lock(locks[lvl]);
        tasks[lvl].push(tp);
        conds[lvl].notify_one();
        return tp;
    }
    
    template<typename T, typename... Args>
    task * 
    enqueue(Args &&...args) noexcept 
    requires std::is_constructible_v<T, Args...> && 
             std::is_base_of_v<task, T>
    {
        task *t = new T(std::forward<Args>(args)...);
        std::lock_guard<std::mutex> lock(locks[0]);
        tasks[0].push(t);
        conds[0].notify_one();
        return t;
    }
};
} // namespace scheduler
#endif
