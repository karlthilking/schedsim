#include <iostream>
#include <random>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include <utility>
#include <algorithm>
#include "task.h"
#include "sjf.h"

#define USE_SJF 0x1

// calculate theoretically optimal average turnaround and average response
// for a given workload
std::pair<float, float>
get_theoretical_opt(const std::vector<task_t> &tasks)
{
    std::sort(begin(tasks), end(tasks), 
        [&](const task_t &t1, const task_t &t2) {
            return t1.get_t_arrival() < t2.get_t_arrival();
        });
}

void 
print_usage()
{
    std::cout << "Usage: ./schedsim [options]\n\n"
              << "Selecting a scheduler to simulate:\n"
              << "\t-s=SCHEDULER or --scheduler=SCHEDULER\n"
              << "Scheduler options:\n"
              << "\t* sjf (Shortest-Job-First Scheduler)\n\n";
}

int 
main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        exit(1);
    }
    int schedopt = 0;
    for (int i{1}; i < argc; ++i) {
        if (!strncmp(argv[i], "-s=sjf", 6)) {
            schedopt |= USE_SJF;
        } else {
            std::cerr << "Unrecognized argument: " << argv[i] << '\n';
            exit(1);
        }
    }
    if (!schedopt) {
        std::cerr << "No scheduler selected. Exiting...\n";
        exit(1);
    }
    
    // set up utilities for random number generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::geometric_distribution<> geo_dist;
    std::uniform_int_distribution<> int_dist(0, 50);
    
    // generate random number of tasks
    int notasks = 0;
    while ((notasks = (int_dist(gen) + 1) % 10) == 0)
        ;
    std::cout << "Scheduling " << notasks << " tasks\n";
   
    std::vector<task_t> tasks;
    tasks.reserve(notasks);
    if (schedopt & USE_SJF) {
        sched::sjf sjf_scheduler;
        for (int i{}; i < notasks; ++i) {
            // add task with a random total runtime
            tasks.emplace_back(ms_t{geo_dist(gen) * 1000 + int_dist(gen)});
            sjf_scheduler.enqueue(&tasks.back());
        }
    }
    float avg_response_t = 0.0f, avg_turnaround_t = 0.0f;
    std::for_each(begin(tasks), end(tasks),
        [&](const task_t &task) {
            avg_response_t += std::chrono::duration_cast<ms_t>(
                task.get_t_firstrun() - task.get_t_arrival()
            ).count();
            avg_turnaround_t += std::chrono::duration_cast<ms_t>(
                task.get_t_completion() - task.get_t_arrival()
            ).count();
        });
    avg_response_t /= (tasks.size() * 1000);
    avg_turnaround_t /= (tasks.size() * 1000);

    std::cout << "Average Response Time: " << avg_response_t << "s"
              << ", Average Turnaround Time: " << avg_turnaround_t << "s\n";
    exit(0);
}
