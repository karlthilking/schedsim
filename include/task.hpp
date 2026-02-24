#ifndef SCHEDSIM_TASK_H
#define SCHEDSIM_TASK_H

#include <cstdint>
#include <sys/types.h>
#include <err.h>
#include <chrono>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <cstring>
#include <cassert>
#include <algorithm>
#include "types.hpp"
#include "random.hpp"

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
    milliseconds                        waittime;

    task_stat() noexcept
        : t_start(high_resolution_clock::now()),
          waittime(milliseconds(0))
    {}

    milliseconds 
    get_t_turnaround() const noexcept
    {
        return duration_cast<milliseconds>(t_completion - t_start);
    }
    milliseconds 
    get_t_response() const noexcept
    {
        return duration_cast<milliseconds>(t_firstrun - t_start);
    }
    milliseconds 
    get_total_waittime() const noexcept
    {
        return waittime + get_t_response();
    }
};

class task {
protected:
    struct rusage   *ru;
    task_stat       *stat;
    pid_t           pid;
    u32             task_id;
    task_state      state;
public:
    task(u32 id) noexcept
        : ru(new struct rusage()),
          stat(new task_stat()),
          pid(0), 
          task_id(id), 
          state(task_state::RUNNABLE)
    {
        stat->t_start = high_resolution_clock::now();
        if (timerisset(&ru->ru_utime)) {
            ru->ru_utime.tv_sec = 0;
            ru->ru_utime.tv_usec = 0;
        } else if (timerisset(&ru->ru_stime)) {
            ru->ru_stime.tv_sec = 0;
            ru->ru_stime.tv_usec = 0;
        }
    }
    
    virtual 
    ~task() noexcept
    {
        delete ru;
        delete stat;
    }
    
    virtual void run() noexcept = 0;

    task_state 
    get_state() const noexcept { return state; }

    void
    set_state(task_state new_state) noexcept { state = new_state; }

    pid_t
    get_pid() const noexcept { return pid; }

    u32
    get_task_id() const noexcept { return task_id; }

    struct rusage * 
    get_rusage() const noexcept { return ru; }
    
    void
    set_rusage(struct rusage *new_ru) noexcept
    {
        memcpy((void *)ru, (void *)new_ru, sizeof(struct rusage));
    }
    
    time_point<high_resolution_clock>
    get_t_start() const noexcept
    {
        return stat->t_start;
    }

    milliseconds
    get_t_turnaround() const noexcept
    {
        assert(state == task_state::FINISHED);
        return stat->get_t_turnaround(); 
    }
    milliseconds
    get_t_response() const noexcept
    {
        assert(state == task_state::FINISHED);
        return stat->get_t_response(); 
    }
    milliseconds
    get_total_waittime() const noexcept
    {
        return stat->get_total_waittime();
    }

    void
    set_t_completion(time_point<high_resolution_clock> t_completion) noexcept
    {
        stat->t_completion = t_completion;
    }
    void
    set_t_firstrun(time_point<high_resolution_clock> t_firstrun) noexcept
    {
        stat->t_firstrun = t_firstrun;
    }
    void 
    set_t_laststop(time_point<high_resolution_clock> stop_time) noexcept
    {
        stat->t_laststop = stop_time;
    }
    void
    increment_waittime(time_point<high_resolution_clock> start_time) noexcept
    {
        stat->waittime += duration_cast<milliseconds>(
            start_time - stat->t_laststop
        );
    }
};

/* cpu bound task */
class cpu_task : public task {
public:
    cpu_task(u32 id) noexcept : task(id) {}
    virtual ~cpu_task() noexcept override {}
    
    virtual void
    run() noexcept override
    {
        pid = fork();
        if (pid < 0)
            err(EXIT_FAILURE, "fork");
        else if (pid > 0)
            return;
        
        std::array<std::array<float, 16>, 16> A;
        std::array<std::array<float, 16>, 16> B;
        
        std::for_each(begin(A), end(A), [&](std::array<float, 16> &row){
            std::generate(begin(row), end(row), [&]{
                return generator::rand<float>(-1024.0f, 1024.0f);
            });
        });
        std::for_each(begin(B), end(B), [&](std::array<float, 16> &row){
            std::generate(begin(row), end(row), [&]{
                return generator::rand<float>(-1024.0f, 1024.0f);
            });
        });

        int N = 1 << 20;
        while (N--) {
            std::array<std::array<float, 16>, 16> C{};
            for (int i = 0; i < 16; ++i)
                for (int k = 0; k < 16; ++k)
                    for (int j = 0; j < 16; ++j)
                        C[i][j] += A[i][k] * B[k][j];
        }
        exit(0);
    }
}; 

/* memory bound task */
class mem_task : public task {
public:
    mem_task(u32 id) noexcept : task(id) {}
    virtual ~mem_task() noexcept override {}
    
    virtual void
    run() noexcept override
    {
        pid = fork();
        if (pid < 0)
            err(EXIT_FAILURE, "fork");
        else if (pid > 0)
            return;
        
        std::vector<std::string> v(4096, "01010");
        int N = 1 << 20;
        while (N--) {
            size_t i1 = generator::rand<size_t>(0, 4095);
            size_t i2 = generator::rand<size_t>(0, 4095);
            [[maybe_unused]] std::string s1 = v[i1];
            [[maybe_unused]] std::string s2 = v[i2];
        }
        exit(0);
    }
};
#endif
