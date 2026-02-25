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
    memcpy(ru, new_ru, sizeof(*ru));
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

    std::array<std::array<float, 16>, 16> A;
    std::array<std::array<float, 16>, 16> B;

    std::for_each(begin(A), end(A), [&](std::array<float, 16> &a){
        std::generate(begin(a), end(a), [&]{
            return generator::rand<float>(-1024.0f, 1024.0f);
        });
    });
    std::for_each(begin(B), end(B), [&](std::array<float, 16> &a){
        std::generate(begin(a), end(a), [&]{
            return generator::rand<float>(-1024.0f, 1024.0f);
        });
    });

    int N = 1 << 15;
    while (N--) {
        std::array<std::array<float, 16>, 16> C{};
        for (int i = 0; i < 16; ++i)
            for (int k = 0; k < 16; ++k)
                for (int j = 0; j < 16; ++j)
                    C[i][j] += A[i][k] * B[k][j];
    }
    exit(0);
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

    std::vector<std::string> v(4096, "01010");
    int N = 1 << 15;
    while (N--) {
        for (int i = 0; i < 16; ++i) {
            size_t i0 = generator::rand<size_t>(0, 4095);
            size_t i1 = generator::rand<size_t>(0, 4095);
            size_t i2 = generator::rand<size_t>(0, 4095);
            size_t i3 = generator::rand<size_t>(0, 4095);
            [[maybe_unused]] std::string s0 = v[i0];
            [[maybe_unused]] std::string s1 = v[i1];
            [[maybe_unused]] std::string s2 = v[i2];
            [[maybe_unused]] std::string s3 = v[i3];
        }
    }
    exit(0);
}

