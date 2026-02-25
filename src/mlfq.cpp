#include <iostream>
#include <queue>
#include <vector>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <cassert>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/types.h>
#include <err.h>
#include "../include/types.hpp"
#include "../include/task.hpp"
#include "../include/mlfq.hpp"

namespace scheduler {
u32
mlfq::cpu_diff(const struct rusage &prev, const struct rusage &cur) 
const noexcept
{
    return ((cur.ru_utime.tv_sec * 1000) + (cur.ru_utime.tv_usec / 1000) +
            (cur.ru_stime.tv_sec * 1000) + (cur.ru_stime.tv_usec / 1000)) -
           ((prev.ru_utime.tv_sec * 1000) + (prev.ru_utime.tv_usec / 1000) +
            (prev.ru_stime.tv_sec * 1000) + (prev.ru_stime.tv_usec / 1000));
}

void
mlfq::schedule(task *t, u32 lvl) noexcept
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
        t->increment_t_waiting(high_resolution_clock::now());
        kill(t->get_pid(), SIGCONT);
    }
    t->set_state(task_state::RUNNING);

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
        t->set_rusage(&cur_ru);
        {
            std::lock_guard<std::mutex> lk(stdout_mtx);
            std::cout << *t << " exited\n";
        }
    } else if (WIFSTOPPED(wstat) && WSTOPSIG(wstat) == SIGSTOP) {
        t->set_state(task_state::STOPPED);
        t->set_t_laststop(high_resolution_clock::now());
        if (cpu_diff(*prev_ru, cur_ru) >= timeslice.count()) {
            t->set_rusage(&cur_ru);
            enqueue(t, (lvl > 0) ? lvl - 1 : lvl);
        } else {
            enqueue(t, lvl);
        }
    } else if (WIFSIGNALED(wstat) && WTERMSIG(wstat) == SIGSTOP) {
        err(EXIT_FAILURE, "SIGSTOP caused child to terminate");
    } else if (WIFSTOPPED(wstat) && WSTOPSIG(wstat) != SIGSTOP) {
        err(EXIT_FAILURE, "child received unexpected stop signal");
    }
}

mlfq::mlfq(u32 ncpus, milliseconds timeslice, 
           u32 nlevels, milliseconds prio_boost_freq) noexcept
    : tasks(std::vector<std::queue<task *>>(nlevels)),
      conds(std::vector<std::condition_variable>(nlevels)),
      locks(std::vector<std::mutex>(nlevels)),
      timeslice(timeslice),
      flag(0)
{
    threads.reserve(ncpus + 1);
    for (u32 cpuid = 0; cpuid < ncpus; ++cpuid) {
        /* scheduler thread */
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
                if (MLFQ_STOP(flag) || MLFQ_HALT(flag))
                    return;
                flag.fetch_or(MLFQ_PRIO_FLAG);
                for (u32 lvl = 1; lvl < tasks.size(); ++lvl) {
                    {
                        std::lock_guard<std::mutex> lk(locks[lvl]);
                        if (tasks[lvl].empty())
                            continue;
                        conds[lvl].notify_all();
                    }
                }
                flag.fetch_xor(MLFQ_PRIO_FLAG);
            }
        } while (1);
    });
}

mlfq::~mlfq() noexcept
{
    if (MLFQ_HALT(flag))
        return;
    flag.fetch_or(MLFQ_STOP_FLAG);
    for (u32 lvl = 0; lvl < tasks.size(); ++lvl) {
        std::lock_guard<std::mutex> lk(locks[lvl]);
        conds[lvl].notify_all();
    }
    for (std::thread &th : threads) {
        assert(th.joinable());
        th.join();
    }
}

void
mlfq::halt() noexcept
{
    flag.fetch_or(MLFQ_HALT_FLAG);
    for (u32 lvl = 0; lvl < tasks.size(); ++lvl) {
        {
            std::lock_guard<std::mutex> lk(locks[lvl]);
            conds[lvl].notify_all();
        }
    }
    for (std::thread &th : threads) {
        assert(th.joinable());
        th.join();
    }
}

void
mlfq::enqueue(task *t, u32 lvl) noexcept
{
    std::lock_guard<std::mutex> lk(locks[lvl]);
    tasks[lvl].push(t);
    conds[lvl].notify_one();
}
} // namespace scheduler
