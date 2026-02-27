#include <chrono>
#include <cstdint>
#include <algorithm>
#include <sys/types.h>
#include <err.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <cassert>
#include "../include/types.hpp"
#include "../include/random.hpp"
#include "../include/task.hpp"

task_stat::task_stat() noexcept
    : t_start(high_resolution_clock::now()),
      t_waiting(milliseconds(0))
{}

milliseconds
task_stat::get_t_turnaround() const noexcept
{
    return duration_cast<milliseconds>(t_completion - t_start);
}

milliseconds
task_stat::get_t_response() const noexcept
{
    return duration_cast<milliseconds>(t_firstrun - t_start);
}

milliseconds
task_stat::get_t_waiting() const noexcept
{
    return t_waiting + get_t_response();
}

task::task(u32 id) noexcept
    : ru(new struct rusage()),
      stat(new task_stat()),
      pid(0),
      task_id(id),
      state(task_state::RUNNABLE)
{
    stat->t_start = high_resolution_clock::now();
}

task::~task() noexcept
{
    delete ru;
    delete stat;
    fclose(procf);
}

task_state
task::get_state() const noexcept
{
    char *line = nullptr;
    size_t sz = 0;
    char state = 'I';
    while (getline(&line, &sz, procf) > 0) {
        if (strncmp(line, "State:", strlen("State:") == 0) {
            if (sscanf(line, "%*s %c", &state) != 1)
                return task_state::INVALID;
            break;
        }
    }
    switch (state) {
    case 'R':
        return task_state::RUNNING;
    case 'S':
        return task_state::SLEEPING;
    case 'D':
        return task_state::DISKSLEEP;
    case 'T':
        return task_state::STOPPED;
    case 'Z':
        return task_state::ZOMBIE;
    case 'X':
        return task_state::FINISHED;
    default:
        return task_state::INVALID;
    }
    free(line);
}

task_state
task::get_state() const noexcept
{
    return state;
}

void
task::set_state(task_state new_state) noexcept
{
    state = new_state;
}

pid_t
task::get_pid() const noexcept
{
    return pid;
}

struct rusage *
task::get_rusage() const noexcept
{
    return ru;
}

void
task::set_rusage(struct rusage *new_ru) noexcept
{
    ru->ru_utime.tv_sec = new_ru->ru_utime.tv_sec;
    ru->ru_utime.tv_usec = new_ru->ru_utime.tv_usec;
    ru->ru_stime.tv_sec = new_ru->ru_stime.tv_sec;
    ru->ru_stime.tv_usec = new_ru->ru_stime.tv_usec;
}

time_point<high_resolution_clock>
task::get_t_start() const noexcept
{
    return stat->t_start;
}

milliseconds
task::get_t_turnaround() const noexcept
{
    assert(state == task_state::FINISHED);
    return stat->get_t_turnaround();
}

milliseconds
task::get_t_response() const noexcept
{
    assert(state != task_state::RUNNABLE);
    return stat->get_t_response();
}

milliseconds
task::get_t_waiting() const noexcept
{
    assert(state != task_state::RUNNABLE);
    return stat->get_t_waiting();
}

void
task::set_t_completion(time_point<high_resolution_clock> t_completion) 
noexcept
{
    stat->t_completion = t_completion;
}

void
task::set_t_firstrun(time_point<high_resolution_clock> t_firstrun)
noexcept
{
    stat->t_firstrun = t_firstrun;
}

void
task::set_t_laststop(time_point<high_resolution_clock> t_stop) 
noexcept
{
    stat->t_laststop = t_stop;
}

void
task::increment_t_waiting(time_point<high_resolution_clock> t_start) 
noexcept
{
    stat->t_waiting += duration_cast<milliseconds>(
        t_start - stat->t_laststop
    );
}

std::ostream &
operator<<(std::ostream &os, const task &t)
{
    switch (t.state) {
    case task_state::RUNNABLE:
        os << "(Task " << t.task_id << ')';
        break;
    default:
        os << "(Task " << t.task_id << ", PID: " << t.pid << ')';
        break;
    }
    return os;
}

cpu_task::cpu_task(u32 id) noexcept : task(id) {}
cpu_task::~cpu_task() noexcept {}

void
cpu_task::run() noexcept
{
    if ((pid = fork()) < 0)
        err(EXIT_FAILURE, "fork");
    else if (pid > 0)
        return;
    
    /* Open /proc/pid/task/pid/status for tracking process state */
    char buf[128];
    snprintf(buf, sizeof(buf), "/proc/%d/task/%d/status", pid, pid);
    if ((procf = fopen(buf, "r")) == NULL)
        err(EXIT_FAILURE, "fopen");

    if (execl("./bin/cpu_task", "./bin/cpu_task", nullptr) < 0)
        err(EXIT_FAILURE, "execvp");
}

mem_task::mem_task(u32 id) noexcept : task(id) {}
mem_task::~mem_task() noexcept {}

void
mem_task::run() noexcept
{
    if ((pid = fork()) < 0)
        err(EXIT_FAILURE, "fork");
    else if (pid > 0)
        return;

    char buf[128];
    snprintf(buf, sizeof(buf), "/proc/%d/task/%d/status", pid, pid);
    if ((procf = fopen(buf, "r") == NULL)
        err(EXIT_FAILURE, "fopen");

    if (execl("./bin/mem_task", "./bin/mem_task", nullptr) < 0)
        err(EXIT_FAILURE, "execvp");
}

