/* generic.cpp */
#include <queue>
#include <cassert>
#include <algorithm>

namespace Generic {
/******************************* FIFO Scheduler *******************************
 *      Schedules first available process to completion wihout premption      *
 ******************************************************************************/

fifo::fifo(const std::vector<task> &tasks) 
    : taskq(begin(tasks), end(tasks), fifocmp{}), t_curr(0)
{        
    stats.reserve(tasks.size());
}

void fifo::start() noexcept
{
    u32 t_curr = 0;
    while (!this->tasks.empty()) {
        float t_response, t_turnaround;
        task t = this->tasks.top();
        this->tasks.pop();  
        if (t.get_state() == task_state::notready && t.t_arrival <= t_curr) {
            t.set_state(task_state::running);
        } else {
            t_curr = t_arrival;
        }
        t_response = t_curr - t.t_arrival;      // response time
        t.rt_curr += t.rt_total;
        t_curr += t.rt_total;
        t_turnaround = t_curr - t.t_arrival;    // turnaround time
        this->stats.emplace_back(t_response, t_turnaround);
    }
}

std::pair<float, float> get_stats() const noexcept
{
    assert(this->all_tasks_done());
    float avg_response_t = 0.0f;
    float avg_turnaround_t = 0.0f;
    for (const auto &stat : stats) {
        avg_response_t += stat.first;
        avg_turnaround_t += stat.second;
    }
    avg_response_t /= ntasks;
    avg_turnaround_t /= ntasks;
    return std::pair<float, float>{ avg_response_t, avg_turnaround_t };
}
/*********************************** FIFO *************************************/


/******************************* SJF Scheduler ********************************
 *      Schedules shortest avaiable job to completion without preemption      *
 ******************************************************************************/
sjf::sjf(const std::vector<task> &tasks)
   : tasks(tasks), tasks_done(0), t_curr(0) {} 

bool sjf::waiting_tasks() const noexcept
{
    return std::any_of(begin(this->tasks), end(this->tasks),
                [](const task &t) { t.t_arrival <= t_curr; });
}

u32 sjf::t_next() const noexcept
{
    u32 next = ~(1 << 31);
    std::for_each(begin(this->tasks), end(this->tasks),
        [](const task &t) { 
            if (!t.state == task_state.finished && t.t_arrival < next) {
                next = t.t_arrival;
            });
    return next;
}

task &sjf::get_sj() const noexcept
{
    task shortest_task;
    u32 min_rt_total = ~(1 << 31);
    for (const task &t : tasks) {
        if (t.t_arrival > t_cur) {
            continue;
        } else if (t.rt_total < min_rt_total) {
            min_rt_total = t.rt_total;
            shortest_task = t;
        }
    }
    return shortest_task;
}

void sjf::start() noexcept
{
    task curr_task;
    while (tasks_done < tasks.size()) {
        task new_task;

        sj = this->get_sj();
    }
}

} // namespace Generic
