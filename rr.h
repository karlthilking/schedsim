#ifndef SCHEDSIM_RR_H
#define SCHEDSIM_RR_H

#include <queue>
#include <vector>
#include <thread>
#include <semaphore>

namespace sched {
class round_robin {
private:
    void
    schedule_task(task_t *task) noexcept
    {
        time_point t_start = hrclock_t::now();

        // task has not been scheduled before
        if (task->get_state() == task_state::RUNNABLE) {
            task->set_t_firstrun(t_start);
        }

        task->set_state(task_state::RUNNING);
        // spin to simulate the task consuming a timeslice
        std::thread task_thread([&]{
            while (std::duration_cast<ms_t>(hrclock_t::now() - t_start)
                   < timeslice)
                ;
            return;
        });
        task_thread.join();

        if ((task->increment_rt_curr(timeslice)) > task->get_rt_total())
            return;

        // if the task is a candidate to be requeued, set its current
        // state to blocked and push it back onto the queue
        task->set_state(task_state::BLOCKED);
        sem.acquire();
        tasks.push(task);
        sem.release();
    }

    std::queue<task_t *>    tasks;
    std::vector<thread>     sched_threads;
    ms_t                    timeslice;
    std::binary_semaphore   sem;
    bool                    stop;
public:
    round_robin(uint32_t ncpus = std::thread::hardware_concurrency(),
                ms_t timeslice = 8ms) noexcept
        : timeslice(timeslice), sem(1), stop(false)
    {
        for (uint32_t i{}; i < ncpus; ++i) {
            sched_thread.emplace_back([this]{
                while (true) {
                    sem.acquire();
                    if (tasks.empty()) {
                        if (stop)
                            return;
                        sem.release();
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

    ~round_robin()
    {
        stop = true;
        for (std::thread &th : threads)
            th.join();
    }
    
    void
    enqueue(task_t &task) noexcept
    {
        sem.acquire();
        tasks.push(task);
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
