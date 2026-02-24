#include <iostream>
#include <iomanip>
#include <random>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include <utility>
#include <algorithm>
#include <fcntl.h>
#include "../include/types.hpp"
#include "../include/task.hpp"
#include "../include/rr.hpp"
#include "../include/mlfq.hpp"
#include "../include/random.hpp"

#define S_RR        0x01
#define S_MLFQ      0x02
#define S_MASK      0x03

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

/*
 *  Display the following metrics of scheduler performance:
 *      - average turnaround time
 *      - average response time
 *      - average waiting time
 *      - cpu utilization
 *      - throughput
 */
void
show_stats(const std::vector<task *> &tasks,
           time_point<high_resolution_clock> t_start, u32 ncpus)
{
    std::cout << "Scheduling Stats:\n";
    auto t_now = high_resolution_clock::now();

    u32 nfinished = 0;
    u32 nstarted = 0;
    milliseconds avg_t_turnaround = 0ms;
    milliseconds avg_t_response = 0ms;
    milliseconds avg_t_waiting = 0ms;
    milliseconds avg_t_running = 0ms;
    seconds t_total = duration_cast<seconds>(t_now - t_start);
    float cpu_utilization = 0.0f;
    float throughput = 0.0f;
    float pct_finished = 0.0f;
    std::for_each(begin(tasks), end(tasks), [&](const task *t){
        /* task never started */
        if (t->get_state() == task_state::RUNNABLE)
            return;
        else if (t->get_state() == task_state::FINISHED) {
            avg_t_running += (t->get_t_turnaround() - t->get_total_waittime());
            avg_t_turnaround += t->get_t_turnaround();
            float t_utilized = (t->get_t_turnaround().count() - 
                                t->get_total_waittime().count());
            cpu_utilization += t_utilized;
            ++nfinished;
            ++throughput;
        } else {
            cpu_utilization +=
                (duration_cast<milliseconds>(t_now - t->get_t_start()).count()
                 - t->get_total_waittime().count());
        }
        ++nstarted;
        avg_t_response += t->get_t_response();
        avg_t_waiting += t->get_total_waittime();
    });
    avg_t_running /= nfinished;
    avg_t_turnaround /= nfinished;
    avg_t_response /= nstarted;
    avg_t_waiting /= nstarted;
    cpu_utilization /= duration_cast<milliseconds>(t_now - t_start).count();
    cpu_utilization /= ncpus;
    throughput /= duration_cast<seconds>(t_now - t_start).count();
    pct_finished = 
        (static_cast<float>(nfinished) / static_cast<float>(nstarted)) * 100;
    std::cout << std::setprecision(4)
              << "Percent Tasks Finished:\t\t" << pct_finished << "%\n"
              << "Total Runtime:\t\t\t" << t_total.count() << "s\n"
              << "Average Task Runtime:\t\t" << avg_t_running.count()
              << "ms\nAverage Turnaround Time:\t" << avg_t_turnaround.count()
              << "ms\nAverage Response Time:\t\t" << avg_t_response.count()
              << "ms\nAverage Wait Time:\t\t" << avg_t_waiting.count()
              << "ms\nCPU Utilization:\t\t" << (cpu_utilization * 100) << '%'
              << "\nThroughput:\t\t\t" << throughput << " (tasks/sec)\n";
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
    seconds runtime = 10s;
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
    if (!(opt & S_MASK)) {
        std::cerr << "No scheduler was selected. Exiting...\n";
        exit(EXIT_FAILURE);
    }
    if (opt & S_RR) {
        /* incomplete for now */
        ;
    }
    if (opt & S_MLFQ) {
        std::vector<task *> tasks;
        tasks.reserve(250);
        time_point<high_resolution_clock> t, t_start, t_end;
        
        scheduler::mlfq mlfq(ncpus, timeslice, nlevels);
        u32 id = 0;
        t_start = high_resolution_clock::now();
        t_end = t_start + runtime;
        for (t = t_start; t < t_end; t = high_resolution_clock::now()) {
            if (generator::rand<int>(0, 1))
                tasks.emplace_back(new cpu_task(id++));
            else
                tasks.emplace_back(new mem_task(id++));
            std::cout << "(Task " << tasks.back()->get_task_id()
                      << ") spawned\n";
            mlfq.enqueue(tasks.back());
            if (generator::rand<int>(0, 3))
                std::this_thread::sleep_for(
                    milliseconds(generator::rand<int>(250, 750))
                );
        }
        mlfq.halt();
        std::cout << "\nSimulation exited\n\n";
        
        show_stats(tasks, t_start, ncpus);
        /* cleanup */
        std::for_each(begin(tasks), end(tasks), [](task *t){ delete t; });
    }
    exit(EXIT_SUCCESS);
}
