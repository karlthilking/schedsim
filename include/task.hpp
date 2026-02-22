#ifndef SCHEDSIM_TASK_H
#define SCHEDSIM_TASK_H

#include <cstdint>
#include <sys/types.h>
#include <err.h>
#include <chrono>
#include <unistd.h>
#include <sys/time.h>
#include "types.hpp"

enum class task_state { 
    RUNNABLE,
    RUNNING,
    STOPPED,
    FINISHED
};

class task {
public:
    struct rusage   ru; // task resource usage
protected:
    pid_t           pid;
    u32             task_id;
    task_state      state;
public:
    task_t(u32 id) : pid(0), task_id(id), state(task_state::RUNNABLE)
    {
        if (timerisset(&ru.ru_utime)) {
            ru.ru_utime.tv_sec = 0;
            ru.ru_utime.tv_usec = 0;
        } else if (timerisset(&ru.ru_stime)) {
            ru.ru_stime.tv_sec = 0;
            ru.ru_stime.tv_usec = 0;
        }
    }
    
    task_t(const task_t &_) = delete;
    task_t &operator=(const task_t &_) = delete;

    task_t(task_t &&other) noexcept
        : ru(std::move(other.ru)),
          pid(std::move(other.pid)),
          task_id(std::move(other.task_id)),
          state(std::move(other.state))
    {}

    task_t &operator=(task_t &&other) noexcept
    {
        ru = std::move(other.ru);
        pid = std::move(other.pid);
        task_id = std::move(other.task_id);
        state = std::move(other.state);
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

    u32
    get_ms_runtime() const noexcept
    {
        return (ru.ru_utime.tv_sec * 1000) + (ru.ru_stime.tv_sec * 1000)
           + (ru.ru_utime.tv_usec / 1000) + (ru.ru_stime.tv_usec / 1000);
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
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dist(-1024.0f, 1024.0f);

            std::generate(begin(A), end(A), [&]{ return dist(gen); });
            std::generate(begin(B), end(B), [&]{ return dist(gen); });
        }
        
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
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 4096);
        
        std::vector<std::string> v(4096, "01010");

        int N = 4096;
        while (N--) {
            std::string &s1 = v[dist(gen)];
            std::string &s2 = v[dist(gen)];
            s1 += s2[0];
            s2 += s1[0];
        }
        exit(0);
    }
};

