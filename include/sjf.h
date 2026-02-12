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
    schedule_task(size_t task_ix) noexcept
    {
        task_t *task = tasks[task_ix];
        tasks.erase(begin(tasks) + task_ix);
        
        task->set_t_firstrun(hrclock_t::now());
        switch (task.set_pid(fork())) {
            case -1:
                err(EXIT_FAILURE, "fork");
            case 0:
                // simulate running for rt_total
                std::this_thread::sleep_for(task.get_rt_total());
            default:
                waitpid(task.getpid(), nullptr, 0);
                task->set_t_completion(hrclock_t::now());
        }
    }
    
    [[nodiscard]] size_t 
    get_shortest_job() const noexcept 
    {
        return std::min_element(begin(tasks), end(tasks),
            [](const task_t *t1, const task_t *t2) {
                return t1->get_t_arrival() < t2->get_t_arrival();
            }) - begin(tasks);
    }
    
    std::vector<task_t *>   tasks;      // schedulable tasks
    std::thread             scheduler;  // scheduler thread
    std::binary_sempahore   sem;        // for locking task list
    bool                    stop;       // prompts scheduler to return
public:
    sjf() : sem(1), stop(false) noexcept
    {
        scheduler = std::thread([this]{
            while (true) {
                sem.acquire();
                if (tasks.empty()) {
                    if (stop)
                        return;
                    sem.release();
                    continue;
                }
                schedule_task(get_shortest_job());
                sem.release();
            }
        });
    }
    
    ~sjf()
    {
        stop = true;
        scheduler.join();
    }
    
    void enqueue(task_t &task) noexcept
    {
        sem.acquire();
        tasks.push_back(&task);
        sem.release();
    }

    void enqueue(task_t *p_task) noexcept
    {
        sem.acquire();
        tasks.push_back(p_task);
        sem.release();
    }
};
} // namespace sched
#endif
