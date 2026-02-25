#ifndef SCHEDSIM_TASK_H
#define SCHEDSIM_TASK_H

#include <chrono>
#include <cstdint>
#include <cstring>
#include <sys/types.h>
#include <sys/resource.h>
#include "types.hpp"

enum class task_state { 
    RUNNABLE,
    RUNNING,
    STOPPED,
    FINISHED
};

struct task_stat {
    time_point<high_resolution_clock>   t_start;
    time_point<high_resolution_clock>   t_firstrun;
    time_point<high_resolution_clock>   t_completion;
    time_point<high_resolution_clock>   t_laststop;
    milliseconds                        t_waiting;

    task_stat() noexcept; 
    milliseconds get_t_turnaround() const noexcept;
    milliseconds get_t_response() const noexcept;
    milliseconds get_t_waiting() const noexcept;
};

class task {
protected:
    struct rusage   *ru;
    task_stat       *stat;
    pid_t           pid;
    u32             task_id;
    task_state      state;
public:
    task(u32 id) noexcept;
    virtual ~task() noexcept;
    virtual void run() noexcept = 0;

    task_state get_state() const noexcept;
    void set_state(task_state new_state) noexcept;

    pid_t get_pid() const noexcept;

    u32 get_task_id() const noexcept;

    struct rusage *get_rusage() const noexcept;
    void set_rusage(struct rusage *new_ru) noexcept;
    
    time_point<high_resolution_clock> get_t_start() const noexcept;

    milliseconds get_t_turnaround() const noexcept;
    milliseconds get_t_response() const noexcept;
    milliseconds get_t_waiting() const noexcept;

    void
    set_t_completion(time_point<high_resolution_clock> t_completion)
    noexcept;

    void
    set_t_firstrun(time_point<high_resolution_clock> t_firstrun) 
    noexcept;

    void 
    set_t_laststop(time_point<high_resolution_clock> t_stop) 
    noexcept;
    
    void
    increment_t_waiting(time_point<high_resolution_clock> t_start) 
    noexcept;
    
    friend std::ostream & 
    operator<<(std::ostream &os, const task &t);
};

class cpu_task : public task {
public:
    cpu_task(u32 id) noexcept;
    virtual ~cpu_task() noexcept override;
    virtual void run() noexcept override;
}; 

class mem_task : public task {
public:
    mem_task(u32 id) noexcept;
    virtual ~mem_task() noexcept override;
    virtual void run() noexcept override;
};
#endif
