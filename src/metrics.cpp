#include <vector>
#include <chrono>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include "../include/task.hpp"
#include "../include/types.hpp"
#include "../include/metrics.hpp"

/*
 *  Dynamic casting a dereferenced base pointer will throw an exception, so
 *  attempting the dynamic cast can confirm the underlying type pointed to
 */
bool
metrics::is_cpu_task(task *t) const noexcept
{
    try {
        dynamic_cast<cpu_task &>(*t);
    } catch (...) {
        return false;
    }
    return true;
}

bool
metrics::is_mem_task(task *t) const noexcept
{
    try {
        dynamic_cast<mem_task &>(*t);
    } catch (...) {
        return false;
    }
    return true;
}

/* Get total task CPU Time */
float
metrics::get_cpu_time(const struct rusage *ru) const noexcept
{
    float cpu_t = static_cast<float>(
        (ru->ru_utime.tv_sec * 1000) + (ru->ru_utime.tv_usec / 1000) +
        (ru->ru_stime.tv_sec * 1000) + (ru->ru_stime.tv_usec / 1000)
    );
    return cpu_t;
}

/* get millisecond time difference between two timeval structures */
float
metrics::get_time_diff(const struct timeval &l, const struct timeval &r) 
const noexcept
{
    return static_cast<float>(
        ((r.tv_sec * 1000) + (r.tv_usec / 1000)) - 
        ((l.tv_sec * 1000) + (l.tv_usec / 1000))
    );
}

metrics::metrics(const std::vector<task *> &tasks, 
                 const struct timeval &t_start) noexcept
    : avg_t_turnaround(0.0f), 
      avg_t_response(0.0f), 
      avg_t_waiting(0.0f), 
      avg_t_running(0.0f), 
      cpu_utilization(0.0f), 
      throughput(0.0f),
      num_tasks(tasks.size()), 
      num_cpu_tasks(0), 
      num_mem_tasks(0),
      avg_rt_cpu_tasks(0.0f), 
      avg_rt_mem_tasks(0.0f),
      t_total(0.0f)
{
    u32 ncpus = get_nprocs();
    struct timeval t_now;
    gettimeofday(&t_now, nullptr);

    t_total = get_time_diff(t_start, t_now);
    
    throughput = static_cast<float>(num_tasks);
    for (task *t : tasks) {
        assert(t->get_state() == task_state::FINISHED);
        
        auto t_turnaround   = t->get_t_turnaround().count();
        auto t_waiting      = t->get_t_waiting().count();
        auto t_running      = t_turnaround - t_waiting;

        avg_t_turnaround    += t_turnaround;
        avg_t_response      += t->get_t_response().count();
        avg_t_waiting       += t_waiting;
        avg_t_running       += t_running;
        
        cpu_utilization     += get_cpu_time(t->get_rusage());
        
        if (is_cpu_task(t)) {
            avg_rt_cpu_tasks += t_running;
            num_cpu_tasks++;
        } else if (is_mem_task(t)) {
            avg_rt_mem_tasks += t_running;
            num_mem_tasks++;
        }
    }
    avg_t_turnaround    /= num_tasks;                   // (1)
    avg_t_response      /= num_tasks;                   // (2)
    avg_t_waiting       /= num_tasks;                   // (3)
    avg_t_running       /= num_tasks;                   // (4)
    cpu_utilization     /= ((t_total * ncpus) / 100);   // (5)
    throughput          /= (t_total / 1000);            // (6)
    avg_rt_cpu_tasks    /= num_cpu_tasks;               // (11)
    avg_rt_mem_tasks    /= num_mem_tasks;               // (12)
    t_total             /= 1000;                        // (13)
}

std::ostream &
operator<<(std::ostream &os, const metrics &m)
{
    os << "Total Scheduler Uptime:\t\t\t" 
       << m.t_total << "s\n"
       << "Total Tasks:\t\t\t\t" 
       << m.num_tasks << '\n'
       << "Total CPU Bound Tasks:\t\t\t" 
       << m.num_cpu_tasks << '\n'
       << "Total Memory Bound Tasks:\t\t" 
       << m.num_mem_tasks << '\n'
       << "Average Turnaround Time:\t\t" 
       << m.avg_t_turnaround << "ms\n"
       << "Average Response Time:\t\t\t" 
       << m.avg_t_response << "ms\n"
       << "Average Waiting Time:\t\t\t" 
       << m.avg_t_waiting << "ms\n"
       << "Average Running Time:\t\t\t" 
       << m.avg_t_running << "ms\n"
       << "CPU Utilization:\t\t\t" 
       << m.cpu_utilization << "%\n"
       << "Throughput:\t\t\t\t" 
       << m.throughput << " tasks/sec\n"
       << "Average Runtime (CPU Bound Tasks):\t" 
       << m.avg_rt_cpu_tasks << "ms\n"
       << "Average Runtime (Memory Bound Tasks):\t" 
       << m.avg_rt_mem_tasks << "ms\n";
    return os;
}
