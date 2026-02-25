#include <iostream>
#include <iomanip>
#include <random>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <thread>
#include <chrono>
#include <utility>
#include <algorithm>
#include <fcntl.h>
#include "../include/types.hpp"
#include "../include/task.hpp"
#include "../include/mlfq.hpp"
#include "../include/random.hpp"
#include "../include/metrics.hpp"
#include "../include/scheduler.hpp"

#define S_RR        0x01                // use round robin scheduler
#define S_MLFQ      0x02                // use multi-level feedback queue
#define S_MASK      0x03                // get only relevant bits
#define S_ANY(flag) ((flag) & S_MASK)   // check for any scheduler selected

void 
print_usage()
{
    std::cout << "Usage: ./schedsim [options]\n\nOptions:\n"
              << "Selecting a Scheduler:\n"
              << "\t-s=SCHEDULER\tSee section on scheduler options\n\n"
              << "Tunable Parameters:\n"
              << "\t-n=N --ncpus=N\t\tSimulate scheduling on N cpus\n"
              << "\t-t=T --timeslice=T\tSimulate a timeslice of T ms\n"
              << "\t-l=L --nlevels=L\tUse L queues (for MLFQ)\n"
              << "\t-r=R --runtime=R\tRun the simulation for R s\n"
              << "\nScheduler Options:\n"
              << "\t* mlfq\t\tMulti-Level Feedback Queue Scheduler\n"
              << "\t* rr\t\tRound Robin Scheduler\n\n";
}

int 
main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        exit(EXIT_FAILURE);
    }
    u8 opt = 0;
    u32 ncpus = 1;
    std::size_t nlevels = 4;
    milliseconds timeslice = 24ms;
    seconds runtime = 30s;
    for (int i = 1; i < argc; ++i) {
        if (!strncmp(argv[i], "-s=rr", 5))
            opt |= S_RR;
        else if (!strncmp(argv[i], "-s=mlfq", 7))
            opt |= S_MLFQ;
        else if (!strncmp(argv[i], "-r=", 3))
            runtime = seconds(std::stoi(argv[i] + 3));
        else if (!strncmp(argv[i], "--runtime=", 10))
            runtime = seconds(std::stoi(argv[i] + 10));
        else if (!strncmp(argv[i], "-n=", 3))
            ncpus = static_cast<u32>(std::stoi(argv[i] + 3));
        else if (!strncmp(argv[i], "--ncpus=", 8))
            ncpus = static_cast<u32>(std::stoi(argv[i] + 8));
        else if (!strncmp(argv[i], "-t=", 3))
            timeslice = milliseconds(std::stoi(argv[i] + 3));
        else if (!strncmp(argv[i], "--timeslice=", 12))
            timeslice = milliseconds(std::stoi(argv[i] + 12));
        else if (!strncmp(argv[i], "-l=", 3))
            nlevels = static_cast<size_t>(std::stoul(argv[i] + 3));
        else if (!strncmp(argv[i], "--nlevels=", 10))
            nlevels = static_cast<size_t>(std::stoul(argv[i] + 10));
        else
            std::cerr << "Unrecognized argument: " << argv[i] << '\n';
    }
    if (!(S_ANY(opt))) {
        std::cerr << "No scheduler was selected. Exiting...\n";
        exit(EXIT_FAILURE);
    }
    if (opt & S_MLFQ)
        scheduler::run<scheduler::mlfq>(runtime, ncpus, nlevels);
    exit(EXIT_SUCCESS);
}
