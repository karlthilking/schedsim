/* rr.cpp Round Robin Scheduler */
#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <semaphore>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <err.h>
#include <cstdint>
#include <cassert>
#include "../include/types.hpp"
#include "../include/task.hpp"
#include "../include/rr.hpp"

namespace scheduler {
void
rr::schedule(task *t) noexcept
{
    assert(t->get_state() != task_state::RUNNING ||
           t->get_state() != task_state::FINISHED);
    
    switch (t->get_state()) {
    case task_state::RUNNABLE:
        t->set_t_firstrun(high_resolution_clock::now());
        t->run();
        std::cout << *t << " started\n";
        break;
    case task_state::STOPPED:
        t->increment_t_waiting(high_resolution_clock::now());
        kill(t->get_pid(), SIGCONT);
        break;
    default:
        err(EXIT_FAILURE, "unexpected task state");
    }
    t->set_state(task_state::RUNNING);

    std::this_thread::sleep_for(timeslice);
    kill(t->get_pid(), SIGSTOP);
    
    struct rusage ru;
    int rc, wstat;
    if ((rc = wait4(t->get_pid(), &wstat, WUNTRACED, &ru)) < 0)
        err(EXIT_FAILURE, "waitpid");
    else if (rc == 0)
        err(EXIT_FAILURE, "child state did not change");
    
    t->set_rusage(&ru);
    if (WIFEXITED(wstat)) {
        t->set_state(task_state::FINISHED);
        t->set_t_completion(high_resolution_clock::now());
        std::cout << *t << " exited\n";
    } else if (WIFSTOPPED(wstat) && WSTOPSIG(wstat) == SIGSTOP) {
        t->set_state(task_state::STOPPED);
        t->set_t_laststop(high_resolution_clock::now());
        enqueue(t);
    }
}

rr::rr(u32 num_cpus, milliseconds timeslice) noexcept
    : timeslice(timeslice), sem(1), flag(0)
{
    threads.reserve(num_cpus);
    for (u32 cpuid = 0; cpuid < num_cpus; ++cpuid) {
        threads.emplace_back([this]{
            while (true) {
                task *t = nullptr;
                sem.acquire();
                if (tasks.empty()) {
                    sem.release();
                    if (RR_STOP(flag))
                        return;
                    continue;
                }
                t = tasks.front();
                tasks.pop();
                sem.release();
                schedule(t);
            }
        });
    }
}

rr::~rr() noexcept
{
    flag = RR_STOP_FLAG;
    for (std::thread &th : threads) {
        assert(th.joinable());
        th.join();
    }
}

void
rr::enqueue(task *t) noexcept
{
    sem.acquire();
    tasks.push(t);
    sem.release();
}
} // namespace scheduler
