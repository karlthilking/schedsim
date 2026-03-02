#ifndef SCHEDSIM_SCHEDULER_H
#define SCHEDSIM_SCHEDULER_H

#include <chrono>
#include <type_traits>
#include <utility>
#include "rr.hpp"
#include "mlfq.hpp"
#include "random.hpp"
#include "metrics.hpp"
#include "task.hpp"

namespace scheduler {
template<typename S, typename... Args>
void 
run(seconds runtime, u32 num_cpus, Args &&...args)
requires std::is_constructible_v<S, u32, Args...>
{
    std::vector<task *> tasks;
    tasks.reserve(100);

    auto t_start = high_resolution_clock::now();
    {
        S s(num_cpus, std::forward<Args>(args)...);
        time_point<high_resolution_clock> t_now;
        u32 id = 0;
        do {
            t_now = high_resolution_clock::now();
            if (!(id % 2))
                tasks.push_back(s.template enqueue<cpu_task>(id));
            else
                tasks.push_back(s.template enqueue<mem_task>(id));
            std::this_thread::sleep_for(
                milliseconds(generator::rand<int>(150, 500))
            );
            ++id;
        } while (duration_cast<seconds>(t_now - t_start) < runtime);
    }
    std::cout << "\nSimulation exited. Obtaining scheduling metrics...\n";
    metrics mt(tasks, num_cpus, t_start);
    std::cout << mt << '\n';

    /* cleanup */
    for (task *t : tasks)
        if (t)
            delete t;
}
} // namespace scheduler
#endif
