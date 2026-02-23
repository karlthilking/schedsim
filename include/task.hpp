#ifndef SCHEDSIM_TASK_H
#define SCHEDSIM_TASK_H

#include <cstdint>
#include <sys/types.h>
#include <err.h>
#include <chrono>
#include <unistd.h>
#include <sys/time.h>
#include <cstring>
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
            ru.ru_utime.tv_sec = 0;
            ru.ru_utime.tv_usec = 0;
        } else if (timerisset(&ru->ru_stime)) {
            ru.ru_stime.tv_sec = 0;
            ru.ru_stime.tv_usec = 0;
        }
    }

    ~task() noexcept
    {
        delete ru;
        delete stat;
    }
    
    task(const task_t &_) = delete;
    task &operator=(const task_t &_) = delete;

    task(task_t &&other) noexcept
        : ru(std::move(other.ru)),
          stat(std::move(other.stat)),
          pid(std::move(other.pid)),
          task_id(std::move(other.task_id)),
          state(std::move(other.state))
    {}

    task &operator=(task_t &&other) noexcept
    {
        if (this != &other) {
            ru = std::move(other.ru);
            stat = std::move(other.stat),
            pid = std::move(other.pid);
            task_id = std::move(other.task_id);
            state = std::move(other.state);
        }
        return this;
    }

    virtual void run() = 0;

    task_state 
    get_state() const noexcept { return state; }

    task_state 
    set_state(task_state new_state) noexcept { state = new_state; }

    pid_t
    get_pid() const noexcept { return pid; }

    u32
    get_task_id() const noexcept { return task_id; }

    const struct rusage &
    get_rusage() const noexcept { return *ru; }
    
    void
    set_rusage(struct rusage &new_ru) noexcept
    {
        memcpy((void *)ru, (void *)&new_ru, sizeof(*ru));
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
    set_t_completion(time_point t_completion) noexcept
    {
        stat->t_completion = t_completion;
    }
    void
    set_t_firstrun(time_point t_firstrun) noexcept
    {
        stat->t_firstrun = t_firstrun;
    }
    void 
    set_t_laststop(time_point stop_time) noexcept
    {
        stat->t_laststop = stop_time;
    }
    void
    increment_waittime(time_point start_time) noexcept
    {
        stat->waittime += (start_time - stat->last_stop);
    }
};

/* cpu bound task */
class cpu_task : public task {
public:
    cpu_task(u32 id) : task(id) {}

    virtual void
    run() override
    {
        pid = fork();
        if (pid < 0)
            err(EXIT_FAILURE, "fork");
        else if (pid > 0)
            return;
        
        std::array<std::array<float, 16>, 16> A;
        std::array<std::array<float, 16>, 16> B;
        
        std::generate(begin(A), end(A), [&]{
            return generator::rand<float>(-1024.0f, 1024.0f);
        });
        std::generate(begin(B), end(B), [&]{
            return generator::rand<float>(-1024.0f, 1024.0f);
        });
        
        int N = 4096;
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
    mem_task(u32 id) : task(id) {}

    virtual void
    run() override
    {
        pid = fork();
        if (pid < 0)
            err(EXIT_FAILURE, "fork");
        else if (pid > 0)
            return;
        
        std::vector<std::string> v(4096, "01010");
        int N = 4096;
        while (N--) {
            size_t i1 = generator::rand<size_t>(0, 4095);
            size_t i2 = generator::rand<size_t>(0, 4095);
            [[maybe_unused]] std::string s1 = v[i1];
            [[maybe_unused]] std::string s2 = v[i2];
        }
        exit(0);
    }
};
