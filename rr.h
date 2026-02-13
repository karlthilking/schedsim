#ifndef SCHEDSIM_RR_H
#define SCHEDSIM_RR_H

#include <queue>
#include <vector>
#include <thread>
#include <semaphore>
#include <algorithm>

namespace sched {
class rr {
private:
    void
    schedule_task(task_t *task) noexcept
    {
        std::cout << "Scheduling " << std::to_string(task->get_taskid())
                  << " (" << task->get_rt_curr() << '/' << task->get_rt_total()
                  << ")\n";
        static time_point t_start{};
        t_start = hrclock_t::now();

        // task has not been scheduled before
        if (task->get_state() == task_state::RUNNABLE) {
            task->set_t_firstrun(t_start);
        }

        task->set_state(task_state::RUNNING);
        // spin to simulate the task consuming a timeslice
        std::thread task_thread([&]{
            std::this_thread::sleep_for(
                std::min(timeslice, task->get_rt_remaining())
            );
            return;
        });
        task_thread.join();

        if ((task->increment_rt_curr(timeslice)) >= task->get_rt_total()) {
            task->set_t_completion(hrclock_t::now());
            return;
        }

        // if the task is a candidate to be requeued, set its current
        // state to blocked and push it back onto the queue
        task->set_state(task_state::BLOCKED);
        sem.acquire();
        tasks.push(task);
        sem.release();
    }

    std::queue<task_t *>        tasks;
    std::vector<std::thread>    sched_threads;
    ms_t                        timeslice;
    std::binary_semaphore       sem;
    bool                        stop;
public:
    rr(uint32_t ncpus = std::thread::hardware_concurrency(),
       ms_t timeslice = 24ms) noexcept
        : timeslice(timeslice), sem(1), stop(false)
    {
        for (uint32_t cpuid{}; cpuid < ncpus; ++cpuid) {
            sched_threads.emplace_back([this] {
                while (true) {
                    do {
                        std::this_thread::yield();
                    } while (!sem.try_acquire());

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
