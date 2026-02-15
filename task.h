#ifndef SCHEDSIM_TASK_H
#define SCHEDSIM_TASK_H

#include <cstdint>
#include <sys/types.h>
#include <err.h>
#include <chrono>
#include "types.h"

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
    u8          task_id;      // for debugging/print statements
    task_state  state;        // current task state
public:
    // t_firstrun, t_completion, and pid do not require initialization
    // because they will be assigned before they are ever used
    task_t(ms_t total, u8 id) noexcept
        : t_arrival(hrclock_t::now()), 
          rt_current(0ms), 
          rt_total(total),
          task_id(id),
          state(task_state::RUNNABLE)
    {}
    
    task_t(const task_t &other)
        : t_arrival(other.t_arrival),
          rt_current(other.rt_current),
          rt_total(other.rt_total),
          task_id(other.task_id),
          state(other.state)
    {}

    task_t(task_t &&other) noexcept
        : t_arrival(std::move(other.t_arrival)),
          rt_current(std::move(other.rt_current)),
          rt_total(other.rt_total),
          task_id(other.task_id),
          state(other.state)
    {}

    task_t &
    operator=(const task_t &other) 
    {
        if (this != &other) {
            t_arrival = other.t_arrival;
            rt_current = other.rt_current;
            rt_total = other.rt_total;
            task_id = other.task_id;
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
            task_id = std::move(other.task_id);
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
    set_t_firstrun(time_point firstrun = hrclock_t::now()) noexcept
    { 
        t_firstrun = firstrun;
    }
    
    time_point
    get_t_completion() const noexcept
    {
        return t_completion;
    }
    void
    set_t_completion(time_point completion = hrclock_t::now()) noexcept
    {
        t_completion = completion;
    }

    ms_t
    increment_rt_curr(ms_t rt_acc) noexcept
    {
        rt_current += rt_acc;
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
   
    void
    set_pid(pid_t new_pid) noexcept 
    { 
        pid = new_pid;
    }

    u8
    get_task_id() const noexcept
    {
        return task_id;
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
    
    // simulate running the task; calling run() begins simulating the process,
    // SIGSTOP is used to the deschedule, SIGCONT is used to reschedule, and
    // SIGKILL should be used when the task is finished
    void
    run()
    {
        pid = fork();
        if (pid < 0)
            err(EXIT_FAILURE, "fork");
        else if (pid > 0)
            return;
        
        // "simulating" the task is just letting it spin; more specific and 
        // varied behavior can be implemented here later on to simulate different
        // and more realistic scheduler workloads
        while (1)
            ;
    }
};

#endif

