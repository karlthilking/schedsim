#include <iostream>
#include <random>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include "../include/task.h"
#include "../include/sjf.h"

enum class sched_type { NONE, SJF };
enum class wkld_type { NONE, SCI, DB };

void print_usage()
{
    std::cout << "Usage: ./schedsim [options]\n\n"
              << "Selecting a scheduler to simulate:\n"
              << "\t-s=SCHEDULER or --scheduler=SCHEDULER\n"
              << "Scheduler options:\n"
              << "\t* sjf (Shortest-Job-First Scheduler)\n\n";
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        exit(1);
    }
    std::string scheduler;
    sched_type sched_choice = sched_type::NONE;
    for (int i{1}; i < argc; ++i) {
        if (!strncmp(argv[i], "-s=", 3)) {
            scheduler = std::string{argv[i] + 3};
            sched_choice = (scheduler == "sjf") ? sched_type::SJF 
                                                : sched_type::NONE;
        } else if (!strncmp(argv[i], "--scheduler=", 12)) {
            scheduler = std::string{argv[i] + 12};
            sched_choice = (scheduler == "sjf") ? sched_type::SJF 
                                                : sched_type::NONE;
        } else {
            std::cerr << "Unrecognized argument: " << argv[i] << '\n';
            exit(1);
        }
    }
    if (sched_choice == sched_type::NONE) {
        std::cerr << "No scheduler selected. Exiting...\n";
        exit(1);
    } else {
        std::cout << "Scheduler: " << scheduler << '\n';
    }
    
    // setup utility for randomization
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<u32> dist(1,100);
   
    // generate random number of tasks and random task arrival times
    u32 n_tasks = dist(gen) % 15 + 5;
    std::cout << "Scheduling " << n_tasks << " tasks\n";
    std::vector<u32> t_arrivals(n_tasks, 0);
    std::generate(begin(t_arrivals), end(t_arrivals), [&]{ return dist(gen); });
    std::sort(begin(t_arrivals), end(t_arrivals));
       
    // initialize shortest-job-first scheduler and enqueue tasks
    sched::sjf sjf{};
    std::for_each(begin(t_arrivals), end(t_arrivals),
        [&](u32 t_arr) {
            u32 rt_total = dist(gen) % 50 + 1;
            sjf.enqueue(task{rt_total, t_arr});
            std::cout << "(task) rt_total: " << rt_total << ", t_arr: "
                      << t_arr << '\n';
        });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    sjf.stop();
    sjf.display_stats();
    exit(0);
}
