#ifndef SCHEDSIM_SJF_H
#define SCHEDSIM_SJF_H

#include <vector>
#include <semaphore>
#include <utility>
#include <algorithm>
#include <unistd.h>
#include <err.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "task.h"

namespace sched {
class sjf {
private:
    void 
    schedule_task(task_t *task) noexcept
    {
        task->set_t_firstrun(hrclock_t::now());
        switch (task->set_pid(fork())) {
            case -1:
                err(EXIT_FAILURE, "fork");
            case 0:
                // simulate running for rt_total
                while (std::chrono::duration_cast<ms_t>(hrclock_t::now() -
                       task->get_t_firstrun()) < task->get_rt_total())
                    ;
                exit(0);
            default:
                if (waitpid(task->get_pid(), nullptr, 0) < 0)
                    err(EXIT_FAILURE, "waitpid");
                task->set_t_completion(hrclock_t::now());
                return;
        }
    }
    
    [[nodiscard]] size_t 
    get_shortest_job() const noexcept 
    {
        size_t min_pos = std::min_element(begin(tasks), end(tasks),
            [](const task_t *t1, const task_t *t2) {
                return t1->get_rt_total() < t2->get_rt_total();
            }) - begin(tasks);
        std::cout << "Scheduling Task " 
                  << std::to_string(tasks[min_pos]->get_taskid())
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
