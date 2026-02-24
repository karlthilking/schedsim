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

#define MLFQ_STOP_FLAG  0x1
#define MLFQ_HALT_FLAG  0x2
#define MLFQ_PRIO_FLAG  0x4
#define MLFQ_STOP(flag) ((flag) & MLFQ_STOP_FLAG)
#define MLFQ_HALT(flag) ((flag) & MLFQ_HALT_FLAG)
#define MLFQ_PRIO(flag) ((flag) & MLFQ_PRIO_FLAG)

namespace scheduler {
class mlfq {
private:
    std::vector<std::queue<task *>>         tasks;
    std::vector<std::thread>                threads;
    std::vector<std::condition_variable>    conds;
    std::vector<std::mutex>                 locks;
    std::mutex                              stdout_mtx;
    milliseconds                            timeslice;
    std::atomic<u8>                         flag;
    
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
    schedule(task *t, u32 lvl) noexcept
    {
        assert(t->get_state() != task_state::RUNNING ||
               t->get_state() != task_state::FINISHED);
        
        struct rusage *prev_ru = t->get_rusage();
        struct rusage cur_ru;

        if (t->get_state() == task_state::RUNNABLE) {
            {
                std::lock_guard<std::mutex> lk(stdout_mtx);
                std::cout << *t << " started\n";
            }
            t->set_t_firstrun(high_resolution_clock::now());
            t->run();
        } else if (t->get_state() == task_state::STOPPED) {
            t->increment_waittime(high_resolution_clock::now());
            kill(t->get_pid(), SIGCONT);
        }
        t->set_state(task_state::RUNNING);
        
        /* let the process run for timeslice ms and then stop */
        std::this_thread::sleep_for(timeslice);
        kill(t->get_pid(), SIGSTOP);

        int rc, wstat;
        if ((rc = wait4(t->get_pid(), &wstat, WUNTRACED, &cur_ru)) < 0)
            err(EXIT_FAILURE, "wait4");
        else if (rc == 0)
            err(EXIT_FAILURE, "child state did not change");
        
        if (WIFEXITED(wstat)) {
            assert(WEXITSTATUS(wstat) == 0);
            t->set_state(task_state::FINISHED);
            t->set_t_completion(high_resolution_clock::now());
            {
                std::lock_guard<std::mutex> lk(stdout_mtx);
                std::cout << *t << " exited with exit code "
                      << WEXITSTATUS(wstat) << '\n';
            }
        } else if (WIFSTOPPED(wstat) && WSTOPSIG(wstat) == SIGSTOP) {
            t->set_state(task_state::STOPPED);
            t->set_t_laststop(high_resolution_clock::now());
            /* 
             *  If the difference between the task's current cpu usage and
             *  previous cpu usage (when it was last demoted or when it was
             *  inserted) has surpassed the duration of a timeslice, it 
             *  should now be demoted
             */
            if (get_ms_diff(*prev_ru, cur_ru) >= timeslice.count()) {
                /* demote queue level and insert there */
                t->set_rusage(&cur_ru);
                enqueue(t, (lvl > 0) ? lvl - 1 : lvl);
            } else {
            /* else keep on the same queue level */
                enqueue(t, lvl);
            }
        } else if (WIFSIGNALED(wstat) && WTERMSIG(wstat) == SIGSTOP) {
            err(EXIT_FAILURE, "STOPSIG caused child to terminate");
        }
        return;
    }

public:
    mlfq(u32 ncpus = std::thread::hardware_concurrency(),
         milliseconds timeslice = 24ms, u32 nlevels = 4,
         [[maybe_unused]] milliseconds prio_boost_freq = 2500ms) noexcept
        : tasks(std::vector<std::queue<task *>>(nlevels)),
          conds(std::vector<std::condition_variable>(nlevels)),
          locks(std::vector<std::mutex>(nlevels)),
          timeslice(timeslice),
          flag(0)
    {
        threads.reserve(ncpus + 1);
        for (u32 cpuid = 0; cpuid < ncpus; ++cpuid) {
            threads.emplace_back([&]{
                while (true) {
                    task *t = nullptr;
                    u32 lvl;
                    for (lvl = 0; lvl < tasks.size(); ++lvl) {
                        std::unique_lock<std::mutex> lk(locks[lvl]);
                        conds[lvl].wait(lk, [&]{
                            return MLFQ_STOP(flag) || MLFQ_HALT(flag) ||
                                   !tasks[lvl].empty() || MLFQ_PRIO(flag);
                        });
                        if (MLFQ_HALT(flag))
                            return;
                        else if (tasks[lvl].empty()) {
                            if (MLFQ_STOP(flag) && lvl == tasks.size() - 1)
                                return;
                            continue;
                        }
                        t = tasks[lvl].front();
                        tasks[lvl].pop();
                        break;
                    }
                    if (t && MLFQ_PRIO(flag))
                        enqueue(t, 0);
                    else if (t)
                        schedule(t, lvl);
                }
            });
        }
        /* priority boost thread */
        threads.emplace_back([&]{
                do {
                    {
                        std::unique_lock<std::mutex> lk(locks[0]);
                        conds[0].wait_for(lk, prio_boost_freq, [&]{
                            return MLFQ_STOP(flag) || MLFQ_HALT(flag);
                        });
                    }
                    if (MLFQ_STOP(flag) || MLFQ_HALT(flag))
                        return;
                    flag.fetch_or(MLFQ_PRIO_FLAG);
                    for (u32 lvl = 1; lvl < tasks.size(); ++lvl) {
                        {
                            std::unique_lock<std::mutex> lk(locks[lvl]);
                            if (tasks[lvl].empty())
                                continue;
                            conds[lvl].notify_all();
                        }
                    }
                    flag.fetch_xor(MLFQ_PRIO_FLAG);
            } while (1);    
        });
    }
    
    /*
     *  when the stop flag is set, threads continue their work until all
     *  tasks are empty before exiting
     */
    ~mlfq() noexcept
    {
        /* 
         *  if mlfq execution was halted, then all threads should have already
         *  been joined
         */
        if (MLFQ_HALT(flag))
            return;
        flag.fetch_or(MLFQ_STOP_FLAG);
        for (u32 lvl = 0; lvl < tasks.size(); ++lvl) {
            std::lock_guard<std::mutex> lk(locks[lvl]);
            conds[lvl].notify_all();
        }
        for (std::thread &th : threads)
            if (th.joinable())
                th.join();
    }
    
    /*
     *  halt will cause threads to immediately exit even if there
     *  remaining work
     */
    void
    halt() noexcept
    {
        flag.fetch_or(MLFQ_HALT_FLAG);
        for (u32 lvl = 0; lvl < tasks.size(); ++lvl) {
            {
                std::lock_guard<std::mutex> lk(locks[lvl]);
                conds[lvl].notify_all();
            }
        }
        for (std::thread &th : threads)
            if (th.joinable())
                th.join();
    }

    /*
     *  By default, a newly created task is insterted into the highest
     *  priority queue (tasks[0]), but enqueue() is also called by
     *  threads reinserting a stopped task into a ready queue, so
     *  the enqueue function allows for specifying a queue level
     */
    void
    enqueue(task *t, u32 lvl = 0) noexcept
    {
        std::lock_guard<std::mutex> lock(locks[lvl]);
        tasks[lvl].push(t);
        conds[lvl].notify_one();
    }

    template<typename T>
    void
    enqueue(T &t, u32 lvl = 0) noexcept
    {
        std::lock_guard<std::mutex> lock(locks[lvl]);
        tasks[lvl].push(new task(t));
        conds[lvl].notify_one();
    }
    
    template<typename T, typename ...Args>
    void
    enqueue(Args &&...args) noexcept
    {
        std::lock_guard<std::mutex> lock(locks[0]);
        tasks[0].push(new task(std::forward<Args>(args)...));
        conds[0].notify_one();
    }

    template<typename It>
    void
    enqueue(It begin, It end, u32 lvl = 0)
    {
        std::lock_guard<std::mutex> lk(locks[lvl]);
        for (auto it = begin; it != end; ++it)
            tasks[lvl].push(*it);
        conds[lvl].notify_all();
    }

    friend std::ostream &
    operator<<(std::ostream &os, mlfq &mlfq_sched);
};

std::ostream &
operator<<(std::ostream &os, mlfq &mlfq_sched)
{
    std::lock_guard<std::mutex> lk(mlfq_sched.stdout_mtx);
    for (u32 lvl = 0; lvl < mlfq_sched.tasks.size(); ++lvl) {
        std::queue<task *> q = mlfq_sched.tasks[lvl];
        os << "MLFQ (Level " << lvl + 1 << "): ";
        while (!q.empty()) {
            os << *q.front() << ' ';
            q.pop();
        }
        os << '\n';
    }
    return os;
}

} // namespace scheduler
#endif
