#include <iostream>
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
#include "../include/sjf.hpp"
#include "../include/rr.hpp"

#define OPT_SJF     0x1  // simulate shortest job first
#define OPT_RR      0x2  // simulate round robin
#define SCHED_MASK  0x3  // check if any scheduler was selected

void 
print_usage()
{
    std::cout << "Usage: ./schedsim [options]\n\nOptions:\n"
              << "Selecting a Scheduler:\n"
              << "\t-s=SCHEDULER\tSee section on scheduler options\n\n"
              << "Tunable Parameters:\n"
              << "\t--ncpus=N\tSimulate scheduling on N cpus\n"
              << "\t--timeslice=T\tSimulate a timeslice of T ms\n"
              << "\nScheduler Options:\n"
              << "\t* sjf\t\tShortest-Job-First Scheduler\n"
              << "\t* rr\t\tRound Robin Scheduler\n\n";
}

u32
randint(u32 lo, u32 hi)
{
    static std::random_device rd{};
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dist(0.0f, 1.0f);

    return static_cast<int>((hi - lo) * dist(gen) + lo);
}

std::vector<ms_t>
generate_runtimes()
{
	u32 ntasks = randint(5, 20);
	std::vector<ms_t> runtimes(ntasks);
	
	std::generate(begin(runtimes), end(runtimes), 
		      [&]{ return randint(100, 2500); });

	return runtimes;
}

template<typename S, typename T>
std::vector<T> *
simulate_scheduler(S &&sched, const std::vector<ms_t> &runtimes)
{
	std::vector<T> *tasks = new std::vector<T>{};
	tasks->reserve(runtimes.size());

	time_point t_start = hrclock_t::now();
	for (u32 i = 0; i < runtimes.size(); ++i) {
		tasks->emplace_back(runtimes[i], i + 1);
		sched.enqueue(tasks->back());

		if (randint(0, 3))
			std::this_thread::sleep_for(randint(250, 500));

		auto t_arrival = std::chrono::duration_cast<ms_t>(
			tasks->back().get_t_arrival() - start
		);

		std::cout << "(Task " << std::to_string(i + 1)
		          << ") Arrival Time: " << t_arrival
			  << ", Total Runtime: "
			  << tasks->back().get_rt_total() << '\n';
	}
	return tasks;
}

int 
main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        exit(1);
    }
    u8 opt;
    // default parameters: cpus=1, timeslice=24ms
    u32 ncpus = 1;     
    ms_t timeslice = 24ms;  
    for (int i{1}; i < argc; ++i) {
        if (!strncmp(argv[i], "-s=sjf", 6)) {
            opt |= OPT_SJF;
        } else if (!strncmp(argv[i], "-s=rr", 5)) {
            opt |= OPT_RR;
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
    
    // vector of random task runtimes
    std::vector<ms_t> runtimes = generate_random_runtimes();

    std::vector<task_t> *tasks;
    if (opt & OPT_SJF) {
        std::cout << "Simulating SJF Scheduler\n";
        tasks = simulate_sched(sched::sjf(ncpus), runtimes);
        report_stats(*tasks);
        delete tasks;
    }
    if (opt & OPT_RR) {
        std::cout << "Simulating Round Robin Scheduler\n";
        tasks = simulate_sched(sched::rr(ncpus, timeslice), runtimes);
        report_stats(*tasks);
        delete tasks;
    }
    exit(0);
}
