#ifndef SCHEDSIM_RR_H
#define SCHEDSIM_RR_H

#include <queue>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cassert>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include "task.hpp"
#include "types.hpp"

#define STOP_FLAG   0x1
#define HALT_FLAG   0x2
#define STOP(flag)  ((flag) & STOP_FLAG)
#define HALT(flag)  ((flag) & HALT_FLAG)

namespace scheduler {
class roundrobin {
private:
    std::queue<task *>          tasks;
    std::vector<std::thread>    threads; // 1 thread = 1 cpu
    std::condition_variable     cv;
    std::mutex                  mtx;
    milliseconds                timeslice;
    u8                          flag;

    void
    schedule(task *t) noexcept
    {
        if (t->get_state() == task_state::RUNNABLE)
            t->run();
        else if (t->get_state() == task_state::STOPPED)
            kill(t->get_pid(), SIGCONT);
        t->set_state(task_state::RUNNING);

        std::this_thread::sleep_for(timeslice);
        kill(t->get_pid(), SIGSTOP);

        int wstat;
        if (waitpid(t->get_pid(), &wstat, WNOHANG) > 0) {
            if (WIFEXITED(wstat)) {
                t->set_state(task_state::FINISHED);
                std::cout << t->get_pid() << " exited with exit code "
                          << WEXITSTATUS(wstat) << '\n';
                return;
            }
        }
        enqueue(t);
    }

public:
    roundrobin(u32 ncpus = 1, milliseconds timeslice = 24ms) noexcept
        : timeslice(timeslice), flag(0)
    {
        threads.reserve(ncpus);
        for (u32 cpuid = 0; cpuid < ncpus; ++cpuid) {
            threads.emplace_back([this]{
                while (true) {
                    task *t;
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [this] {
                            return STOP(flag) || HALT(flag) ||
                                   !(tasks.empty());
                        });
                        if (HALT(flag) || (STOP(flag) && tasks.empty()))
                            return;
                        t = tasks.front();
                        tasks.pop();
                    }
                    schedule(t);
                }
            });
        }
    }

    ~roundrobin() noexcept
    {
        flag |= STOP_FLAG;
        cv.notify_all();
        for (std::thread &thrd : threads)
            thrd.join();
    }

    void
    halt() noexcept
    {
        flag = HALT_FLAG;
        cv.notify_all();
        for (std::thread &thrd : threads)
            thrd.join();
    }
    
    void
    enqueue(task *t) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(t);
        cv.notify_one();
    }

    template<typename T>
    void
    enqueue(T &&t) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(new task(t));
        cv.notify_one();
    }

    template<typename T, typename ...Args>
    void
    enqueue(Args &&...args) noexcept
    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(new task(std::forward<Args>(args)...));
        cv.notify_one();
    }
};
} // namespace scheduler
