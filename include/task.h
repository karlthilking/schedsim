#include <cstdint>

enum class task_state {
    NOTREADY, SLEEPING, BLOCKED, RUNNABLE, RUNNING, FINISHED 
};

class task {
public:
    using u32 = uint32_t;
private:
    u32         rt_curr;    // current runtime
    u32         rt_total;   // total runtime
    u32         t_arr;      // arrival time
    task_state  state;      // process state
public:
    constexpr task() = default;
    constexpr task(u32 rt_total_, u32 t_arr_, task_state state_);

    constexpr u32 get_rt_curr() const noexcept;
    constexpr void set_rt_curr(u32 new_rt_curr) noexcept;

    constexpr u32 get_rt_total() const noexcept;
    constexpr void set_rt_total(u32 new_rt_total) noexcept;

    constexpr u32 get_t_arr() const noexcept;
    constexpr void set_t_arr(u32 new_t_arr) noexcept;

    constexpr task_state get_state() const noexcept;
    constexpr void set_state(task_state new_state) noexcept;
};


