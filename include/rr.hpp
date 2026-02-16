#ifndef SCHEDSIM_RR_H
#define SCHEDSIM_RR_H

#include <queue>
#include <vector>
#include <thread>
#include <semaphore>
#include <algorithm>
#include <cassert>
#include <unistd.h>
#include <signal.h>
#include "task.hpp"
#include "types.hpp"

namespace sched {
class rr {
private:
    void
    schedule_task(task_t *task) noexcept
    {
        if (task->get_state() == task_state::BLOCKED)
            assert(task->get_pid() > 0 && task->get_pid() != getpid());
        assert(task->get_state() != task_state::NOTREADY);
        
        time_point t_start = hrclock_t::now();
        ms_t t_quantum = std::min(timeslice, task->get_rt_remaining());
        
        if (task->get_state() == task_state::RUNNABLE) {
            task->set_t_firstrun(t_start);
            task->run();
        } else if (task->get_state() == task_state::BLOCKED) {
            kill(task->get_pid(), SIGCONT);
        }
        task->set_state(task_state::RUNNING);

        std::this_thread::sleep_for(t_quantum);
        time_point t_end = hrclock_t::now();
        
        if (task->increment_rt_curr(t_quantum) >= task->get_rt_total()) {
            task->set_t_completion(t_end);
            kill(task->get_pid(), SIGKILL);
            std::cout << "Task " << std::to_string(task->get_task_id())
                      << " finished\n";
        } else {
            kill(task->get_pid(), SIGSTOP);
            enqueue(task);
        }
    }

    std::queue<task_t *>        tasks;
    std::vector<std::thread>    sched_threads;
    ms_t                        timeslice;
    std::binary_semaphore       sem;
    bool                        stop;
public:
    rr(u32 ncpus = 1, ms_t timeslice_ = 24ms) noexcept
        : timeslice(timeslice_), sem(1)
    {
        for (u32 cpuid{}; cpuid < ncpus; ++cpuid) {
            sched_threads.emplace_back([this] {
                while (true) {
                    sem.acquire();
                    if (tasks.empty()) {
                        sem.release();
                        if (stop)
                            return;
                        continue;
                    }
                    task_t *task = tasks.front();
                    tasks.pop();
                    sem.release();
                    schedule_task(task);
                }
            });
        }
    }

    ~rr()
    {
        stop = true;
        for (std::thread &th : sched_threads) {
            th.join();
        }
    }
    
    void
    enqueue(task_t &task) noexcept
    {
        sem.acquire();
        tasks.push(&task);
        sem.release();
    }

    void
    enqueue(task_t *task) noexcept
    {
        sem.acquire();
        tasks.push(task);
        sem.release();
    }
};
} // namespace sched
#endif
