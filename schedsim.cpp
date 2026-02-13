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
    std::uniform_int_distribution<> int_dist(0, 25);
    
    // generate random number of tasks
    uint8_t notasks = 10;
    std::cout << "Scheduling " << notasks << " tasks\n";
   
    std::vector<task_t> tasks;
    tasks.reserve(notasks);
    time_point sched_start = hrclock_t::now();
    if (schedopt & USE_SJF) {
        sched::sjf sjf_scheduler;
        for (int i{}; i < notasks; ++i) {
            // add task with a random total runtime
            tasks.emplace_back(ms_t{int_dist(gen) * 100 + int_dist(gen)});
            sjf_scheduler.enqueue(&tasks.back());
            if (!(int_dist(gen) % 2))
                std::this_thread::sleep_for(ms_t{int_dist(gen)});
        }
    }
    int taskno = 1;
    for (const task_t &t : tasks) {
        std::cout << "(Task " << taskno++ << ") Arrival Time: "
                  << std::chrono::duration_cast<ms_t>(t.get_t_arrival() -
                     sched_start) << ", Total Runtime: " 
                  << t.get_rt_total() << '\n';
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
    avg_response_t /= tasks.size();;
    avg_turnaround_t /= tasks.size();

    std::cout << "Average Response Time: " << avg_response_t << "ms"
              << ", Average Turnaround Time: " << avg_turnaround_t << "ms\n";
    exit(0);
}
