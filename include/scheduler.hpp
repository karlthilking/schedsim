#ifndef SCHEDSIM_SCHEDULER_H
#define SCHEDSIM_SCHEDULER_H

#include <chrono>
#include <type_traits>
#include <utility>
#include "mlfq.hpp"
#include "random.hpp"
#include "metrics.hpp"
#include "task.hpp"

namespace scheduler {
template<typename S>
concept is_scheduler = requires(S &s, task *t, u32 id)
{
    s.enqueue(t);
    s.template enqueue<cpu_task>(id);
    s.template enqueue<mem_task>(id);
};

template<typename S>
constexpr bool is_scheduler_v = is_scheduler<S>;

template<typename S, typename... Args>
void 
run(seconds runtime, u32 num_cpus, Args &&...args)
requires is_scheduler_v<S> && std::is_constructible_v<S, u32, Args...>
{
    std::vector<task *> tasks;
    tasks.reserve(100);

    auto t_start = high_resolution_clock::now();
    auto t_end = t_start + runtime;
    {
        S s(num_cpus, std::forward<Args>(args)...);
        for (u32 id = 0; high_resolution_clock::now() < t_end; ++id) {
            if (generator::rand<float>(0.0f, 1.0f) >= 0.5)
                tasks.push_back(s.template enqueue<cpu_task>(id));
            else
                tasks.push_back(s.template enqueue<mem_task>(id));
            std::this_thread::sleep_for(
                milliseconds(generator::rand<int>(250, 600))
            );
        }
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
