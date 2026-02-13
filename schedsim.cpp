#include <iostream>
#include <random>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include <utility>
#include <algorithm>
#include <fcntl.h>
#include "task.h"
#include "sjf.h"
#include "rr.h"

#define OPT_SJF     0x1  // simulate shortest job first
#define OPT_RR      0x2  // simulate round robin
#define OPT_SL      0x4  // minimal/silent output 
#define OPT_VB      0x8  // verbose output
#define SCHED_MASK  0x3  // check if any scheduler was selected

void 
print_usage()
{
    std::cout << "Usage: ./schedsim [options]\n\nOptions:\n"
              << "Selecting a Scheduler:\n"
              << "\t-s=SCHEDULER\tSee section on scheduler options\n\n"
              << "Tunable Parameters:\n"
              << "\t--ncpus=N\tSimulate scheduling on N cpus\n"
              << "\t--timeslice=T\tSimulate a timeslice of T milliseconds\n"
              << "\nScheduler Options:\n"
              << "\t* sjf\t\tShortest-Job-First Scheduler\n"
              << "\t* rr\t\tRound Robin Scheduler\n\n";
}

int
rand_gen()
{
    static std::random_device rd{};
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(0, 25);
    return dist(gen);
}

std::vector<ms_t>
random_rt_gen()
{
    int ntasks((rand_gen() % 25) + 10);
    std::cout << "Running " << std::to_string(ntasks) << " tasks\n";

    std::vector<ms_t> rts;
    rts.reserve(ntasks);
    
    for (int i{}; i < ntasks; ++i)
        rts.emplace_back(ms_t{rand_gen() * 100 + rand_gen()});
    return rts;
}

template<typename S>
std::vector<task_t> *
simulate_sched(S &&sched, const std::vector<ms_t> &runtimes, uint8_t opt)
{
    std::vector<task_t> *tasks = new std::vector<task_t>{};
    tasks->reserve(runtimes.size());

    time_point start = hrclock_t::now();
    for (uint32_t i{}; i < runtimes.size(); ++i) {
        tasks->emplace_back(runtimes[i], i + 1);
        sched.enqueue(tasks->back());

        // add random sleep so tasks dont all arrive at the same time
        if (rand_gen() % 3)
            std::this_thread::sleep_for(ms_t{rand_gen() * 5});

        auto t_arrival = std::chrono::duration_cast<ms_t>(
            tasks->back().get_t_arrival() - start
        );
        if (!(opt & OPT_SL)) {
            std::cout << "(Task " << std::to_string(i + 1) << ") Arrival Time: "
                << t_arrival << ", Total Runtime: "
                << tasks->back().get_rt_total() << '\n';
        }
    }
    return tasks;
}

void
report_stats(const std::vector<task_t> &tasks)
{
    ms_t avg_response_t = 0ms, avg_turnaround_t = 0ms;
    std::for_each(begin(tasks), end(tasks),
        [&](const task_t &task) {
            avg_response_t += std::chrono::duration_cast<ms_t>(
                task.get_t_firstrun() - task.get_t_arrival()
            );
            avg_turnaround_t += std::chrono::duration_cast<ms_t>(
                task.get_t_completion() - task.get_t_arrival()
            );
        });
    avg_response_t /= tasks.size();
    avg_turnaround_t /= tasks.size();

    std::cout << "Average Response Time: " << avg_response_t
              << ", Average Turnaround Time: " << avg_turnaround_t << '\n';
    return;
}

int 
main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        exit(1);
    }
    uint8_t opt = 0;
    // default parameters: cpus=1, timeslice=48ms
    uint32_t ncpus = 1;     
    ms_t timeslice = 48ms;  
    for (int i{1}; i < argc; ++i) {
        if (!strncmp(argv[i], "-s=sjf", 6)) {
            opt |= OPT_SJF;
        } else if (!strncmp(argv[i], "-s=rr", 5)) {
            opt |= OPT_RR;
        } else if (!strncmp(argv[i], "--silent", 8)) {
            opt |= OPT_SL;
        } else if (!strncmp(argv[i], "--verbose", 9)) {
            opt |= OPT_VB;
        } else if (!strncmp(argv[i], "--ncpus=", 8)) {
            ncpus = std::stoi(argv[i] + 8);
        } else if (!strncmp(argv[i], "--timeslice=", 12)) {
            timeslice = ms_t{std::stoi(argv[i] + 12)};
        } else {
            std::cerr << "Unrecognized argument: " << argv[i] << '\n';
            exit(1);
        }
    }
    if (!(opt & SCHED_MASK)) {
        std::cerr << "No scheduler selected. Exiting...\n";
        exit(1);
    }
    
    std::vector<ms_t> rand_rts = random_rt_gen();
    std::vector<task_t> *tasks;
    if (opt & OPT_SJF) {
        std::cout << "Simulating SJF Scheduler\n";
        tasks = simulate_sched(sched::sjf{ncpus}, rand_rts, opt);
        report_stats(*tasks);
        delete tasks;
    }
    if (opt & OPT_RR) {
        std::cout << "Simulating Round Robin Scheduler\n";
        tasks = simulate_sched(sched::rr{ncpus, timeslice, opt}, rand_rts, opt);
        report_stats(*tasks);
        delete tasks;
    }
    exit(0);
}
