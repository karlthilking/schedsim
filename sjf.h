#ifndef SCHEDSIM_SJF_H
#define SCHEDSIM_SJF_H

#include <vector>
#include <semaphore>
#include <utility>
#include <algorithm>
#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <signal.h>
#include <cassert>
#include "task.h"

namespace sched {
class sjf {
private:
    void 
    schedule_task(task_t *task) noexcept
    {
        assert(task->get_state() == task_state::RUNNABLE);

        task->set_t_firstrun(hrclock_t::now());
        task->run();
        task->set_state(task_state::RUNNING);
        
        std::this_thread::sleep_for(task->get_rt_total());
        
        task->set_t_completion(hrclock_t::now());
        kill(task->get_pid(), SIGKILL);
        task->set_state(task_state::FINISHED);
    }
    
    [[nodiscard]] size_t 
    get_shortest_job() const noexcept 
    {
        size_t min_pos = std::min_element(begin(tasks), end(tasks),
            [](const task_t *t1, const task_t *t2) {
                return t1->get_rt_total() < t2->get_rt_total();
            }) - begin(tasks);
        std::cout << "Scheduling Task " 
                  << std::to_string(tasks[min_pos]->get_task_id())
                  << ", Runtime: " << tasks[min_pos]->get_rt_total() << '\n';
        return min_pos;
    }
    
    std::vector<task_t *>       tasks;          // schedulable tasks
    std::vector<std::thread>    sched_threads;  // scheduler thread
    std::binary_semaphore       sem;            // for locking task list
    bool                        stop;           // prompts scheduler to return
public:
    sjf(uint32_t ncpus = 1) noexcept 
        : sem(1), stop(false)
    {
        sched_threads.reserve(ncpus);
        for (uint32_t cpuid{}; cpuid < ncpus; ++cpuid) {
            sched_threads.emplace_back([this]{
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
                    
                    size_t ix = get_shortest_job();
                    task_t *task = tasks[ix];
                    tasks.erase(begin(tasks) + ix);
                    sem.release();
                    schedule_task(task);
                }
            });
        }
    }
    
    ~sjf()
    {
        stop = true;
        for (std::thread &th : sched_threads)
            th.join();
    }
    
    void 
    enqueue(task_t &task) noexcept
    {
        sem.acquire();
        tasks.push_back(&task);
        sem.release();
    }

    void 
    enqueue(task_t *task) noexcept
    {
        sem.acquire();
        tasks.push_back(task);
        sem.release();
    }
};
} // namespace sched
#endif
