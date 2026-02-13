#ifndef SCHEDSIM_RR_H
#define SCHEDSIM_RR_H

#include <queue>
#include <vector>
#include <thread>
#include <semaphore>
#include <algorithm>

#define OPT_SL    0x4
#define OPT_VB    0x8

namespace sched {
class rr {
private:
    void
    schedule_task(task_t *task) noexcept
    {
        // verbose output
        if (opt & OPT_VB) {
            std::cout << "Scheduling " << std::to_string(task->get_taskid())
                      << " (" << task->get_rt_curr() << '/' 
                      << task->get_rt_total() << ")\n";
        }
        static time_point t_start{};
        t_start = hrclock_t::now();

        // task has not been scheduled before
        if (task->get_state() == task_state::RUNNABLE)
            task->set_t_firstrun(t_start);

        task->set_state(task_state::RUNNING);
        std::thread task_thread([&]{
            if (task->get_rt_remaining() < timeslice) {
                std::this_thread::sleep_for(task->get_rt_remaining());
                return;
            }
            std::this_thread::sleep_for(timeslice);
            return;
        });

        if ((task->increment_rt_curr(timeslice)) >= task->get_rt_total()) {
            if (!(opt & OPT_SL)) {
                std::cout << "Task " << std::to_string(task->get_taskid())
                          << " exited (" << task->get_rt_curr() << '/'
                          << task->get_rt_total() << ")\n";
            }
            task->set_t_completion(hrclock_t::now());
            task_thread.join();
            return;
        }

        // if the task is a candidate to be requeued, set its current
        // state to blocked and push it back onto the queue
        task->set_state(task_state::BLOCKED);
        task_thread.join();
        sem.acquire();
        tasks.push(task);
        sem.release();
    }

    std::queue<task_t *>        tasks;
    std::vector<std::thread>    sched_threads;
    ms_t                        timeslice;
    std::binary_semaphore       sem;
    const uint8_t               opt;
    bool                        stop;
public:
    rr(uint32_t ncpus = 1, ms_t timeslice_ = 24ms, uint8_t opt_ = 0) noexcept
        : timeslice(timeslice_), sem(1), opt(opt_)
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
