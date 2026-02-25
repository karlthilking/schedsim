#ifndef SCHEDSIM_RR_H
#define SCHEDSIM_RR_H

#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <semaphore>
#include <unistd.h>
#include <type_traits>
#include <sys/types.h>
#include <cstdint>
#include "types.hpp"
#include "task.hpp"

#define RR_STOP_FLAG    0x1
#define RR_STOP(flag)   ((flag) & RR_STOP_FLAG)

namespace scheduler {
class rr {
private:
    std::queue<task *>          tasks;      // 40 bytes
    std::vector<std::thread>    threads;    // 24 bytes
    milliseconds                timeslice;  // 8 bytes
    std::binary_semaphore       sem;        // 4 bytes
    u8                          flag;       // 1 byte

    void schedule(task *t) noexcept;
public:
    rr(u32 num_cpus, milliseconds timeslice) noexcept;
    ~rr() noexcept;

    void enqueue(task *t) noexcept;
    
    /*
     *  Heap allocate new task pointer of derived type and return the
     *  pointer to the caller to manage deallocation
     */
    template<typename T, typename... Args>
    task *
    enqueue(Args &&...args) noexcept
    requires std::is_constructible_v<T, Args...> && 
             std::is_base_of_v<task, T>
    {
        task *t = new T(std::forward<Args>(args)...);
        sem.acquire();
        tasks.push(t);
        sem.release();
        return t;
    }
};
} // namespace scheduler
#endif
