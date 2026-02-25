#ifndef SCHEDSIM_METRICS_H
#define SCHEDSIM_METRICS_H

#include <vector>
#include <chrono>
#include "types.hpp"
#include "task.hpp"

/*  Scheduling Metrics:
 *      - (1)  Average Turnaround Time (finish time - start time)
 *      - (2)  Average Response Time (first run - start time)
 *      - (3)  Average Time Spent Waiting (time spent in ready queue)
 *      - (4)  Average Time Spent Running (time spent executing)
 *      - (5)  CPU Utilization (cpu time / total uptime)
 *      - (6)  Throughput (tasks per second)
 *      - (7)  Scheduling Fairness
 *  Other Metrics:
 *      - (8)  Total Number of Tasks
 *      - (9)  Total Number of CPU Bound Tasks
 *      - (10) Total Number of Memory Bound Tasks
 *      - (11) Average Runtime for All Tasks
 *      - (12) Average Runtime for CPU Bound Tasks
 *      - (13) Average Runtime for Memory Bound Tasks
 *      - (14) Total Simulation Uptime (seconds)
 */
class metrics {
private:
    float avg_t_turnaround; // 1
    float avg_t_response;   // 2
    float avg_t_waiting;    // 3
    float avg_t_running;    // 4
    float cpu_utilization;  // 5
    float throughput;       // 6

    u32 num_tasks;          // 8
    u32 num_cpu_tasks;      // 9
    u32 num_mem_tasks;      // 10
    
    float avg_rt_all_tasks; // 11
    float avg_rt_cpu_tasks; // 12
    float avg_rt_mem_tasks; // 13 

    float t_total;          // 14

    /* helper functions */
    bool is_cpu_task(task *t) const noexcept;
    bool is_mem_task(task *t) const noexcept;
    float get_cpu_time(struct rusage *ru) const noexcept;
public:
    metrics(const std::vector<task *> &tasks, u32 num_cpus,
            time_point<high_resolution_clock> t_start) noexcept;
    
    friend std::ostream &
    operator<<(std::ostream &os, const metrics& m);
};
#endif
