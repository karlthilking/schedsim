#ifndef SCHEDSIM_SCHEDULER_H
#define SCHEDSIM_SCHEDULER_H

#include <sys/time.h>
#include <type_traits>
#include <utility>
#include <unistd.h>
#include "rr.hpp"
#include "mlfq.hpp"
#include "random.hpp"
#include "metrics.hpp"
#include "task.hpp"

namespace scheduler {
/* return number of currently available cpus on this system */
// int
// get_ncpus()
// {
//     int fd;
//     fd = open("/sys/devices/system/cpu/online", O_RDONLY);
//     char buf[5];
//     read(fd, buf, sizeof(buf) - 1);
//     close(fd);
//     return (buf[2] - '0') * 10 + (buf[3] - '0') + 1;
// }

template<typename S, typename... Args>
void 
run(u32 runtime, Args &&...args) requires std::is_constructible_v<S, Args...>
{
    std::vector<task *> tasks;
    tasks.reserve(100);

    struct timeval t_start, t_cur, t_end;
    gettimeofday(&t_start, nullptr);
    t_end.tv_sec = t_start.tv_sec + runtime;
    {
        S s(std::forward<Args>(args)...);
        for (u32 id = 0; ; ++id) {
            gettimeofday(&t_cur, nullptr);
            if (t_cur.tv_sec > t_end.tv_sec)
                break;

            if (id % 2)
                tasks.push_back(s.template enqueue<cpu_task>(id));
            else
                tasks.push_back(s.template enqueue<mem_task>(id));

            usleep(generator::rand<u32>(150000, 500000));
        }
    }
    std::cout << "\nSimulation exited. Obtaining scheduling metrics...\n";
    metrics mt(tasks, t_start);
    std::cout << mt << '\n';

    /* cleanup */
    for (task *t : tasks)
        if (t)
            delete t;
}
} // namespace scheduler
#endif
