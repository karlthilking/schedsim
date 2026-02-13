#ifndef SCHEDSIM_TASK_H
#define SCHEDSIM_TASK_H

#include <cstdint>
#include <sys/types.h>
#include <chrono>

using namespace std::literals;
using hrclock_t     = std::chrono::high_resolution_clock;
using time_point    = std::chrono::time_point<hrclock_t>;
using ms_t          = std::chrono::milliseconds;
using sec_t         = std::chrono::seconds;

enum class task_state {
    NOTREADY    = 0,    // task can not be scheduled yet
    SLEEPING    = 1,    // task is sleeping (performing I/O)
    BLOCKED     = 2,    // task is blocked (descheduled/evicted)
    RUNNABLE    = 3,    // task is runnable
    RUNNING     = 4,    // task is running
    FINISHED    = 5     // task exited
};

class task_t {
private:
    time_point  t_arrival;    // time of arrival
    time_point  t_firstrun;   // time of first run
    time_point  t_completion; // time of completion
    ms_t        rt_current;   // current accumulated runtime
    ms_t        rt_total;     // total task runtime
    pid_t       pid;          // process id
    uint8_t     taskid;       // for debugging/print statements
    task_state  state;        // current task state
public:
    // t_firstrun, t_completion, and pid do not require initialization
    // because they will be assigned before they are ever used
    task_t(ms_t total, uint8_t id) noexcept
        : t_arrival(hrclock_t::now()), 
          rt_current(0ms), 
          rt_total(total),
          taskid(id),
          state(task_state::RUNNABLE) {}
    
    task_t(const task_t &other)
        : t_arrival(other.t_arrival),
          rt_current(other.rt_current),
          rt_total(other.rt_total),
          taskid(other.taskid),
          state(other.state)
    {}

    task_t(task_t &&other) noexcept
        : t_arrival(std::move(other.t_arrival)),
          rt_current(std::move(other.rt_current)),
          rt_total(other.rt_total),
          taskid(other.taskid),
          state(other.state)
    {}

    task_t &
    operator=(const task_t &other) 
    {
        if (this != &other) {
            t_arrival = other.t_arrival;
            rt_current = other.rt_current;
            rt_total = other.rt_total;
            taskid = other.taskid;
            state = other.state;
        }
        return *this;
    }

    task_t &
    operator=(task_t &&other) noexcept
    {
        if (this != &other) {
            t_arrival = std::move(other.t_arrival);
            rt_current = std::move(other.rt_current);
            rt_total = std::move(other.rt_total);
            taskid = std::move(other.taskid);
            state = std::move(other.state);
        }
        return *this;
    }
    
    time_point 
    get_t_arrival() const noexcept 
    { 
        return t_arrival; 
    }
    
    time_point
    get_t_firstrun() const noexcept
    {
        return t_firstrun;
    }
    void
    set_t_firstrun(time_point firstrun) noexcept
    { 
        t_firstrun = firstrun;
    }
    
    time_point
    get_t_completion() const noexcept
    {
        return t_completion;
    }
    void
    set_t_completion(time_point completion) noexcept
    {
        t_completion = completion;
    }

    ms_t
    increment_rt_curr(ms_t rt_acc) noexcept
    {
        if ((rt_current += rt_acc) > rt_total) {
            rt_current = rt_total;
            state = task_state::FINISHED;
        }
        return rt_current;
    }
    ms_t
    get_rt_curr() const noexcept
    {
        return rt_current;
    }
    ms_t
    get_rt_remaining() const noexcept
    {
        return rt_total - rt_current;
    }

    ms_t
    get_rt_total() const noexcept 
    { 
        return rt_total; 
    }

    pid_t
    get_pid() const noexcept 
    { 
        return pid; 
    }
    
    // Returning the pid after setting is for the convenience of doing
    // something like this:
    // switch (task.set_pid(fork())) {}
    pid_t
    set_pid(pid_t new_pid) noexcept 
    { 
        pid = new_pid;
        return pid;
    }

    uint8_t
    get_taskid() const noexcept
    {
        return taskid;
    }

    task_state
    get_state() const noexcept 
    { 
        return state; 
    }
    void
    set_state(task_state new_state) noexcept 
    { 
        state = new_state; 
    }
};

#endif

