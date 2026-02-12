#ifndef SCHEDSIM_SJF_H
#define SCHEDSIM_SJF_H

#include <queue>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <utility>
#include "task.h"

namespace sched {
class sjf {
private:
    enum class flag_t { START, STOP, NONE };
    struct cmp {
        bool operator()(const task &t1, const task &t2)
        {
            return t2.get_t_arr() < t1.get_t_arr();
        }
    };
    void schedule(task &curr_task) noexcept;
private:
    std::priority_queue<task, std::vector<task>, cmp>   tasks;
    std::vector<std::pair<float, float>>                stats;
    std::condition_variable                             cv;
    std::mutex                                          tasks_mutex;
    std::thread                                         sched_thread;
    u32                                                 t_curr;
    flag_t                                              flag;
public:
    sjf();
    sjf(const std::vector<task> &tasks_);
    ~sjf();

    void enqueue(task &&new_task) noexcept;
    void start() noexcept;
    void stop() noexcept;
    void display_stats() const noexcept;
};
} // namespace sched
#endif
