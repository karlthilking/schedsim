#include <iostream>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <cstdlib>
#include "../include/types.hpp"
#include "../include/task.hpp"
#include "../include/rr.hpp"
#include "../include/mlfq.hpp"
#include "../include/random.hpp"
#include "../include/metrics.hpp"
#include "../include/scheduler.hpp"

#define S_RR    0x01 // use round robin scheduler
#define S_MLFQ  0x02 // use multi-level feedback queue

void 
print_usage()
{
    std::cout << "Usage: ./schedsim [options]\n\nOptions:\n"
              << "Selecting a Scheduler:\n"
              << "\t-s=SCHEDULER\tSee section on scheduler options\n\n"
              << "Tunable Parameters:\n"
              << "\t-r R\tRun the simulation for R s\n"
              << "\nScheduler Options:\n"
              << "\t* mlfq\t\tMulti-Level Feedback Queue Scheduler\n"
              << "\t* rr\t\tRound Robin Scheduler\n\n";
}

int 
main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        _exit(EXIT_FAILURE);
    }

    u8 opt = 0x00;
    u32 runtime = 15;

    for (int i = 1; i < argc; ++i) {
        if (!strncmp(argv[i], "-s=rr", 5))
            opt |= S_RR;
        else if (!strncmp(argv[i], "-s=mlfq", 7))
            opt |= S_MLFQ;
        else if (!strncmp(argv[i], "-r", 2)) {
            if (i + 1 == argc)
                std::cerr << "A runtime value must be provided after -r\n";
            else
                runtime = strtoul(argv[i + 1], nullptr, 10);
        } else
            std::cerr << "Unrecognized Argument: " << argv[i] << '\n';
    }

    if (!opt) {
        std::cerr << "No scheduler was selected. Exiting...\n";
        _exit(EXIT_FAILURE);
    }
    
    if (opt & S_RR)
        scheduler::run<scheduler::rr>(runtime);
    else if (opt & S_MLFQ)
        scheduler::run<scheduler::mlfq>(runtime);
    
    _exit(EXIT_SUCCESS);
}
