#ifndef SCHEDSIM_MLFQ_H
#define SCHEDSIM_MLFQ_H

#include <array>
#include <queue>
#include <vector>
#include <thread>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include "types.hpp"
#include "task.hpp"

#define STOP_FLAG   0x1
#define PAUSE_FLAG  0x2
#define HALT_FLAG   0x4
#define STOP(flag)  ((flag) & STOP_FLAG)
#define PAUSE(flag) ((flag) & PAUSE_FLAG)
#define HALT(flag)  ((flag) & HALT_FLAG)

namespace scheduler {
template<u32 N>  // N = number of queues
class mlfq {
private:
    std::array<std::queue<task *>, N> tasks;
    std::vector<std::thread>                         threads;
    std::array<std::condition_variable, N>           conds;
    std::array<std::mutex, N>                        locks;
    milliseconds                                     timeslice;
    u8                                               flag;
    
    /*  
     *  Get the user and system CPU usage difference beetween
     *  the previous and current rusage struct in milliseconds;
     *  when diff >= timeslice the task should be demoted to a 
     *  lower queue level
     */
    u32
    get_ms_diff(const struct rusage &prev, const struct rusage &cur) 
    const noexcept
    {
        u32 prev_ms = (prev.ru_utime.tv_sec * 1000) + 
                      (prev.ru_utime.tv_usec / 1000) +
                      (prev.ru_stime.tv_sec * 1000) + 
                      (prev.ru_stime.tv_usec / 1000);
        u32 cur_ms = (cur.ru_utime.tv_sec * 1000) +
                     (cur.ru_utime.tv_usec / 1000) +
                     (cur.ru_stime.tv_sec * 1000) +
                     (cur.ru_stime.tv_usec / 1000);
        return cur_ms - prev_ms;
    }
    
    /*  
     *  Run the task for a timeslice and, unless the tasks exits after
     *  its timeslice, determine at which queue level to reinsert the
     *  task
     */
    void
    schedule(task *t, u32 level) noexcept
    {
        assert(t->get_state() != task_state::RUNNING ||
               t->get_state() != task_state::FINISHED);
        
        const struct rusage &prev_ru = t->get_rusage();
        struct rusage cur_ru;

        if (t->get_state() == task_state::RUNNABLE) {
            t->set_t_firstrun(high_resolution_clock::now());
            t->run();
        } else if (t->get_state() == task_state::STOPPED) {
            t->increment_waittime(high_resolution_clock::now());
            kill(t->get_pid(), SIGCONT);
        }
        t->set_state(task_state::RUNNING);

        std::this_thread::sleep_for(timeslice);
        kill(t->get_pid(), SIGSTOP);

        int rc, wstat;
        if ((rc = wait4(t->get_pid(), &wstat, 0, &cur_ru)) < 0)
            err(EXIT_FAILURE, "wait4");
        else if (rc == 0)
            err(EXIT_FAILURE, "child state did not change");
        
        if (WIFEXITED(wstat)) {
            t->set_state(task_state::FINISHED);
            t->set_t_completion(high_resolution_clock::now());
            std::cout << t->get_pid() << " exited with exit code "
                      << WEXITSTATUS(wstat) << '\n';
        } else if (WIFSTOPPED(wstat)) {
            t->set_state(task_state::STOPPED);
            t->set_laststop(high_resolution_clock::now());
            if (get_ms_diff(prev_ru, cur_ru) >= timeslice.count()) {
                /* demote queue level and insert there */
                t->set_rusage(cur_ru);
                enqueue(t, level - 1);
            } else {
            /* else keep on the same queue level */
                enqueue(t, level);
            }
        }
        return;
    }

public:
    mlfq(u32 ncpus = 1, milliseconds timeslice = 24ms,
         milliseconds prio_boost_freq = 2500ms) noexcept
        : timeslice(timeslice), flag(0)
    {
        threads.reserve(ncpus);
        for (u32 cpuid = 0; cpuid < ncpus; ++cpuid) {
            threads.emplace_back([this]{
                while (true) {
                    task *t;
                    u32 level;
                    for (level = 0; level < N; ++level) {
                        std::unique_lock lock(locks[level]);
                        conds[level].wait(lock, [this]{
                            return ((STOP(flag) || !(tasks.empty())) &&
                                   (!(PAUSE(flag)))) || HALT(flag);
                        });
                        if (HALT(flag))
                            return;
                        if (tasks[level].empty()) {
                            if (STOP(flag) && level == N - 1)
                                return;
                            continue;
                        }
                        t = tasks[level].front();
                        tasks[level].pop();
                        break;
                    }
                    schedule(t, level);
                }
            });
        }
        /* priority boost thread */
        threads.emplace_back([this]{
            while (!(STOP(flag) || HALT(flag))) {
                std::this_thread::sleep_for(prio_boost_freq);
                flag |= PAUSE_FLAG;
                for (u32 level = 1; level < N; ++level) {
                    {
                        std::scoped_lock lock(locks[0], locks[level]);
                        while (!tasks[level].empty()) {
                            tasks[0].push(tasks[level].front());
                            tasks[level].pop();
                        }
                    }
                    conds[level].notify_all();
                }
                flag ^= PAUSE_FLAG;
                conds[0].notify_all();
            }
        });
    }
    
    /*
     *  when the stop flag is set, threads continue their work until all
     *  tasks are empty before exiting
     */
    ~mlfq() noexcept
    {
        flag |= STOP_FLAG;
        for (std::condition_variable &cv : conds)
            cv.notify_all();
        for (std::thread &thrd : threads) {
            if (thrd.joinable())
                thrd.join();
        }
    }
    
    /*
     *  halt will cause threads to immediately exit even if there
     *  remaining work
     */
    void
    halt() noexcept
    {
        flag = HALT_FLAG;
        for (std::condition_variable &cv : conds)
            cv.notify_all();
        for (std::thread &thrd : threads)
            thrd.join();
    }

    /*
     *  By default, a newly created task is insterted into the highest
     *  priority queue (tasks[0]), but enqueue() is also called by
     *  threads reinserting a stopped task into a ready queue, so
     *  the enqueue function allows for specifying a queue level
     */
    void
    enqueue(task *t, u32 level = 0) noexcept
    {
        std::lock_guard<std::mutex> lock(locks[level]);
        tasks[level].push(t);
        conds[level].notify_one();
    }

    template<typename T>
    void
    enqueue(T &t, u32 level = 0) noexcept
    {
        std::lock_guard<std::mutex> lock(locks[level]);
        tasks[level].push(new task(t));
        conds[level].notify_one();
    }
    
    template<typename T, typename ...Args>
    void
    enqueue(Args &&...args) noexcept
    {
        std::lock_guard<std::mutex> lock(locks[0]);
        tasks[0].push(new task(std::forward<Args>(args)...));
        conds[0].notify_one();
    }
};
} // namespace scheduler
