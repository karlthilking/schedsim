/* generic.h */
#include <queue>
#include <vector>
#include <utility>
#include <cstdint>

namespace Generic {
using u8    = uint8_t;
using u16   = uint16_t;
using u32   = uint32_t;
using u64   = uint64_t;
using i8    = int8_t;
using i16   = int16_t;
using i32   = int32_t;
using i63   = int64_t;

/* Multi-Level Feedback Queue Scheduler */
// class mlfq {
//     ;
// };

/* First-In, First-Out Scheduler */
class fifo {
private:
    struct fifocmp {
        bool operator()(task &t1, task &t2) const noexcept
        {
            return t1.t_arrival > t2.t_arrival;
        }
    };
    std::priority_queue<task, std::vector<task>, fifocmp>   taskq;
    std::vector<std::pair<float, float>>                    stats;
    u32                                                     t_curr;
public:
    fifo(const std::vector<task> &tasks);
    void start() noexcept;
    std::pair<float, float> get_stats() const noexcept;
};

/* Shortest-Job-First Scheduler */
class sjf {
private:
    std::vector<task> tasks;
    u32 tasks_done;
    u32 t_curr;
    bool waiting_tasks() const noexcept;
    u32 t_next() const noexcept;
    task &get_sj() const noexcept;
public:
    sjf(const std::vector<task> &tasks);
    void start() noexcept;
    std::pair<float, float> get_stats() const noexcept;
};

/* Shortest-Time-To-Completion-First Scheduler */
class stcf {
private:
    std::vector<task> tasks;
    u32 tasks_done;
    task &get_stc() const noexcept;
    bool all_tasks_done() const noexcept;
public:
    stcf(const std::vector<task> &tasks);
    void start() noexcept;
    std::pair<float, float> get_stats() const noexcept;
};

} // namespace Generic
