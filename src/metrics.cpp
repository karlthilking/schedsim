#include <vector>
#include <chrono>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <iomanip>
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
metrics::get_cpu_time(struct rusage *ru) const noexcept
{
    float cpu_t = static_cast<float>(
        (ru->ru_utime.tv_sec * 1000) + (ru->ru_utime.tv_usec / 1000) +
        (ru->ru_stime.tv_sec * 1000) + (ru->ru_stime.tv_usec / 1000)
    );
    return cpu_t;
}

metrics::metrics(const std::vector<task *> &tasks, u32 num_cpus,
                 time_point<high_resolution_clock> t_start) noexcept
    : avg_t_turnaround(0.0f), 
      avg_t_response(0.0f), 
      avg_t_waiting(0.0f), 
      avg_t_running(0.0f), 
      cpu_utilization(0.0f), 
      throughput(0.0f),
      num_tasks(0), 
      num_cpu_tasks(0), 
      num_mem_tasks(0),
      avg_rt_all_tasks(0.0f), 
      avg_rt_cpu_tasks(0.0f), 
      avg_rt_mem_tasks(0.0f),
      t_total(0.0f)
{
    auto t_now = high_resolution_clock::now();
    t_total = duration_cast<milliseconds>(t_now - t_start).count();
    
    num_tasks = tasks.size();
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
        
        avg_rt_all_tasks    += t_running;
        if (is_cpu_task(t)) {
            avg_rt_cpu_tasks += t_running;
            num_cpu_tasks++;
        } else if (is_mem_task(t)) {
            avg_rt_mem_tasks += t_running;
            num_mem_tasks++;
        }
    }
    avg_t_turnaround    /= num_tasks;                       // (1)
    avg_t_response      /= num_tasks;                       // (2)
    avg_t_waiting       /= num_tasks;                       // (3)
    avg_t_running       /= num_tasks;                       // (4)
    cpu_utilization     /= ((t_total * num_cpus) / 100);    // (5)
    throughput          /= (t_total / 1000);                // (6)
    avg_rt_all_tasks    /= num_tasks;                       // (11)
    avg_rt_cpu_tasks    /= num_cpu_tasks;                   // (12)
    avg_rt_mem_tasks    /= num_mem_tasks;                   // (13)
    t_total             /= 1000;                            // (14)
}

std::ostream &
operator<<(std::ostream &os, const metrics &m)
{
    os << std::setprecision(4)
       << "Total Scheduler Uptime:\t\t\t" 
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
       << "Average Runtime:\t\t\t" 
       << m.avg_rt_all_tasks << "ms\n"
       << "Average Runtime (CPU Bound Tasks):\t" 
       << m.avg_rt_cpu_tasks << "ms\n"
       << "Average Runtime (Memory Bound Tasks):\t" 
       << m.avg_rt_mem_tasks << "ms\n";
    return os;
}
