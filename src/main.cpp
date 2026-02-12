#include <iostream>
#include <random>
#include <cstdlib>
#include <cstring>
#include "../include/task.h"
#include "../include/sjf.h"

enum class sched { NONE, SJF, STCF };
enum wkload { NONE, SCI, DB };

void print_usage()
{
    std::cout << "Usage: ./schedsim [options]\n\n"
              << "Selecting a scheduler to simulate:\n"
              << "\t-s=SCHEDULER or --scheduler=SCHEDULER\n"
              << "Scheduler options:\n"
              << "\t* sjf (Shortest-Job-First Scheduler)\n\n"
              << "Selecting a workload to simulate:\n"
              << "\t-w=WORKLOAD or --workload=WORKLOAD\n"
              << "Workload options:\n"
              << "\t* n/a\n\n";
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        exit(1);
    }
    std::string scheduler;
    std::string workload;
    sched sched_choice = sched::NONE;
    wkload wkload_choice = wkload::NONE;
    for (int i = 1; i < argc; ++i) {
        if (!strncmp(argv[i], "-s=", 3)) {
            scheduler = std::string{argv[i] + 3};
            sched_choice = (scheduler == "sjf") ? sched::SJF :
                           (scheduler == "stcf") ? sched::STCF :
                           sched::NONE;
        } else if (!strncmp(argv[i], "--scheduler=", 12)) {
            if (sched_choice != sched::NONE) {
                std::cout << "Overriding previous scheduler selection: "
                          << scheduler << '\n';
            }
            scheduler = std::string{argv[i] + 12};
            sched_choice = (scheduler == "sjf") ? sched::SJF :
                           (scheduler == "stcf") ? sched::STCF :
                           sched::NONE;
        } else if (!strncmp(argv[i], "-w=", 3)) {
            workload = std::string{argv[i] + 3};
            wkload_choice = (workload == "sci") ? wkload::SCI :
                            (workload == "db") ? wkload::DB :
                            wkload::NONE;
        } else if (!strncmp(argv[i], "--workload=", 11)) {
            if (wkload_choice != wkload::NONE) {
                std::cout << "Overriding previous workload selection: "
                          << workload << '\n';
            }
            workload = std::string{argv[i] + 11};
            wkload_choice = (workload == "sci") ? wkload::SCI :
                            (workload == "db") ? wkload::DB :
                            wkload::NONE;
        } else {
            std::cerr << "Unrecognized argument: " << argv[i] << '\n';
            exit(1);
        }
    }
    if (sched_choice == sched::NONE) {
        std::cerr << "No schedulder selected for simulation. Exiting...\n";
        exit(1);
    } else if (wkload_choice == wkload::NONE) {
        std::cerr << "No workload selected for simulation. Exiting...\n";
        exit(1);
    } else {
        std::cout << "Selected scheduler: " << scheduler
                  << "\nSelected workload: " << workload << '\n';
    }
    exit(0);
}
