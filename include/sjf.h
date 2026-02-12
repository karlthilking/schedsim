#include <queue>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <utility>
#include "task.h"

class sjf {
private:
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
    u32                                                 t_curr;
    bool                                                done;
public:
    sjf();
    sjf(const std::vector<task> &tasks_);
    
    void enqueue() noexcept;
    void stop() noexcept;
    void display_stats() const noexcept;
};
