#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <array>

void
print_usage() 
{
    std::cout << "Usage: ./schedsim [options]\n"
              << "Options:\n" << "Simulate Schedulers:\n"
              << "\t-cfs (Linux CFS)\n"
              << "\t-O1 (Linux O1 Scheduler)\n"
              << "\t-ule (FreeBSD ULE)\n"
              << "\t-mlfq (MLFQ Scheduler)\n"
              << "\t-fifo (FIFO Scheduler)\n"
              << "\t-rr (Round Robin Scheduler)\n"
              << "\t-ss (Stride Scheduler)\n"
              << "\t-ls (Lottery Scheduler)\n"
              << "Simulated Workloads:\n"
              << "\t-db (Simulate Database Workload)\n"
              << "\t-sci (Simulate Scientific Computing Workload)\n";
}

int
main(int argc, char *argv[])
{
    static const std::array<std::string, 8> sched_options = {
        "-cfs", "-O1", "-ule", "-mlfq", "-fifo", "-rr", "-ss", "-ls" 
    };
    static const std::array<std::string, 2> wl_options = {
        "-db", "-sci"
    };
    if (argc < 2) {
        print_usage();
        exit(1);
    } else if (argv[2] == "-h" || argv[2] == "--help") {
        print_usage();
        exit(0);
    }
    for (int i = 1; i < argc; ++i) {
        for (const std::string &sched : schedulers) {
            if (argv[i] == sched) {
            
            }
        }
        for (const std::string &wl : workloads) {
            if (argv[i] == wl) {

            }
        }
    }
    exit(0);
}
