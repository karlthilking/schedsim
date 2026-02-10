#include <cassert>
#include "../include/task.h"

task::task(unsigned int total, task_state state) noexcept
    : rt_curr(0), rt_total(total), state(state) {}

unsigned int 
task::get_rt_curr() const noexcept
{
    return this->rt_curr;
}

void
task::set_rt_curr(unsigned int new_rt_curr) noexcept
{
    this->rt_curr = new_rt_curr;
}

unsigned int
task::get_rt_total() const noexcept
{
    return this->rt_total;
}

void
task::set_rt_total(unsigned int new_rt_total) noexcept
{
    this->rt_total = new_rt_total;
}
