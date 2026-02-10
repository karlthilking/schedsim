/* task.h */
#include <cstdint>

/* task that simulates a process as part of a scheduling workload */
enum class task_state {
    notready    = 0,
    ready       = 1,
    running     = 2,
    blocked     = 3,
    finished    = 4,
};

class task {
private:
    using u32 = uint32_t;
private:
    u32         rt_curr;
    u32         rt_total;
    u32         t_arrival;
    task_state  state;
public:
    task(u32 total, u32 arrival, task_state state) noexcept;
    u32 get_rt_curr() const noexcept;
    void set_rt_curr(u32 new_rt_curr) noexcept;
    u32 get_rt_total() const noexcept;
    void set_rt_total(u32 new_rt_total) noexcept;
    void set_state(task_state state) noexcept;
    task_state get_state(task_state state);
};

