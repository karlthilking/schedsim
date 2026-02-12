#ifndef SCHEDSIM_TASK_H
#define SCHEDSIM_TASK_H
#include <cstdint>

using u32 = uint32_t;
enum class task_state {
    NOTREADY    = 0, 
    SLEEPING    = 1, 
    BLOCKED     = 2, 
    RUNNABLE    = 3, 
    RUNNING     = 4, 
    FINISHED    = 5
};

class task {
private:
    u32         rt_curr;    // current runtime
    u32         rt_total;   // total runtime
    u32         t_arr;      // arrival time
    task_state  state;      // process state
public:
    constexpr task() = default;
    
    constexpr task(u32 rt_total_, u32 t_arr_) noexcept
        : rt_curr(0), rt_total(rt_total_), 
          t_arr(t_arr_), state(task_state::NOTREADY) {}

    constexpr task(u32 rt_total_, u32 t_arr_, task_state state_) noexcept
        : rt_curr(0), rt_total(rt_total_), t_arr(t_arr_), state(state_) {}

    constexpr u32 get_rt_curr() const noexcept 
    { 
        return this->rt_curr; 
    }
    constexpr void set_rt_curr(u32 new_rt_curr) noexcept
    {
        this->rt_curr = new_rt_curr;
    }

    constexpr u32 get_rt_total() const noexcept
    {
        return this->rt_total;
    }
    constexpr void set_rt_total(u32 new_rt_total) noexcept
    {
        this->rt_total = new_rt_total;
    }

    constexpr u32 get_t_arr() const noexcept
    {
        return this->t_arr;
    }
    constexpr void set_t_arr(u32 new_t_arr) noexcept
    {
        this->t_arr = new_t_arr;
    }

    constexpr task_state get_state() const noexcept
    {
        return this->state;
    }
    constexpr void set_state(task_state new_state) noexcept
    {
        this->state = new_state;
    }
};

#endif

