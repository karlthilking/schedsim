#ifndef SCHEDSIM_TASK_H
#define SCHEDSIM_TASK_H

#include <cstdint>
#include <sys/types.h>
#include <err.h>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>
#include "types.hpp"

enum class task_state { 
    RUNNABLE,
    RUNNING,
    STOPPED,
    SLEEPING,
    ZOMBIE
};

class task_t {
private:
    pid_t       pid;
    task_state  state;
    void        *addr;
public:
    task_t() : pid(0), state(task_state::RUNNABLE) {}
    
    task_t(const task_t &other)
        : pid(other.pid), state(other.state)
    {}

    task_t &operator=(const task_t &other)
    {
        pid = other.pid;
        state = other.state;
    }

    task_t(task_t &&other) noexcept
        : pid(std::move(other.pid)), state(std::move(other.state))
    {}

    task_t &operator=(task_t &&other) noexcept
    {
        pid = std::move(other.pid);
        state = std::move(other.state);
    }

    virtual void run() = 0;
    
    void
    sighandler(int sig)
    {
        struct timespec tp;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
        memcpy(addr, (void *)&tp, sizeof(tp));
        raise(SIGSTOP);
    }
};

// cpu bound task
class cpu_task_t : public task_t {
private:
    pid_t       pid;
    task_state  state;
public:
    virtual void
    run() override
    {
        pid = fork();
        if (pid < 0)
            err(EXIT_FAILURE, "fork");
        else if (pid > 0)
            return;
        
        signal(SIGUSR2, sighandler);
        std::array<std::array<float, 16>, 16> A;
        std::array<std::array<float, 16>, 16> B;
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dist(-1024.0f, 1024.0f);

            std::generate(begin(A), end(A), [&]{ return dist(gen); });
            std::generate(begin(B), end(B), [&]{ return dist(gen); });
        }
        
        /*  Repeatedly multiply matrices to simulate a cpu / computationally
         *  bounded tasks; matrix sizes are kept relatively small to ensure
         *  the simulated task does not become memory bound at all
         */
        while (num_iterations--) {
            std::array<std::array<float, 16>, 16> C{};
            
            for (int i = 0; i < 16; ++i)
                for (int k = 0; k < 16; ++k)
                    for (int j = 0; j < 16; ++j)
                        C[i][j] += A[i][k] * B[k][j];
        }
        exit(0);
    }
}; 

// memory bound task
class mem_task_t : public task_t {
private:
    pid_t       pid;
    task_state  state;
public:
    virtual void
    run() override
    {
        pid = fork();
        if (pid < 0)
            err(EXIT_FAILURE, "fork");
        else if (pid > 0)
            return;
        
        signal(SIGUSR2, sighandler);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 4096);
        
        /*  Random access pattern will cause the cpu to repeatedly
         *  wait for memory from RAM that it does not have it a cache
         */
        std::vector<std::string> v(4096, "12345");
        while (num_iterations--) {
            [[maybe_unused]] auto s = v[dist(gen)];
        }
        exit(0);
    }
};

