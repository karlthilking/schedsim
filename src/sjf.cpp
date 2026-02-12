#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <utility>
#include <iostream>
#include <algorithm>
#include <condition_variable>
#include "../include/sjf.h"
#include "../include/task.h"

namespace sched {
void sjf::schedule(task &curr_task) noexcept
{
    if (curr_task.get_state() != task_state::RUNNABLE &&
        curr_task.get_t_arr() > this->t_curr) {
        this->t_curr = curr_task.get_t_arr();
    }
    curr_task.set_state(task_state::RUNNING);
    float t_response = 
        static_cast<float>(t_curr) - static_cast<float>(curr_task.get_t_arr());
    this->t_curr += curr_task.get_rt_total();
    curr_task.set_rt_curr(curr_task.get_rt_total());
    curr_task.set_state(task_state::FINISHED);
    this->stats.emplace_back(
        static_cast<float>(t_curr) - static_cast<float>(curr_task.get_t_arr()),
        t_response
    );
    return;
}

sjf::sjf()
    : t_curr(0), flag(flag_t::NONE)
{
    sched_thread = std::thread([this] {
        while (true) {
            task curr_task;
            {
                std::unique_lock<std::mutex> lock(tasks_mutex);
                cv.wait(lock, [this] { 
                    return this->flag == flag_t::STOP || !(this->tasks.empty());
                });
                if (this->flag == flag_t::STOP && this->tasks.empty()) {
                    return;
                }
                curr_task = this->tasks.top();
                this->tasks.pop();
            }
            this->schedule(curr_task);
        }
    });
}

sjf::~sjf() { sched_thread.join(); }

sjf::sjf(const std::vector<task> &tasks_)
    : tasks(begin(tasks_), end(tasks_)), t_curr(0), flag(flag_t::NONE)
{
    u32 no_imm_tasks = tasks.size();
    sched_thread = std::thread([&] {
        while (no_imm_tasks--) {
            task curr_task;
            {
                std::scoped_lock lock(this->tasks_mutex);
                if (this->flag == flag_t::STOP && this->tasks.empty()) {
                    return;
                }
                curr_task = this->tasks.top();
                this->tasks.pop();
            }
            this->schedule(curr_task);
        }
        while (true) {
            task curr_task;
            {
                std::unique_lock<std::mutex> lock(tasks_mutex);
                cv.wait(lock, [this] {
                    return this->flag == flag_t::STOP || !(this->tasks.empty());
                });
                if (this->flag == flag_t::STOP && this->tasks.empty()) { 
                    return; 
                }
                curr_task = this->tasks.top();
                this->tasks.pop();
            }
            this->schedule(curr_task);
        }
    });
}

void sjf::enqueue(task &&new_task) noexcept 
{
    std::scoped_lock lock(this->tasks_mutex);
    if (this->t_curr > new_task.get_t_arr() && 
        new_task.get_state() != task_state::RUNNABLE) {
        new_task.set_state(task_state::RUNNABLE);
    } else if (new_task.get_state() != task_state::NOTREADY) {
        new_task.set_state(task_state::NOTREADY);
    }
    this->tasks.push(new_task);
    this->cv.notify_one();
}

void sjf::start() noexcept
{
    flag = flag_t::START;
    this->cv.notify_one();
}

void sjf::stop() noexcept 
{ 
    flag = flag_t::STOP;
    this->cv.notify_one();
}

void sjf::display_stats() const noexcept
{
    float avg_turnaround = 0.0f, avg_response = 0.0f;
    std::for_each(begin(this->stats), end(this->stats),
        [&](const auto &stat) {
            avg_turnaround += stat.first;
            avg_response += stat.second;
        });
    avg_turnaround /= this->stats.size();
    avg_response /= this->stats.size();
    std::cout << "Average Turnaround Time: " << avg_turnaround
              << ", Average Response Time: " << avg_response << std::endl;
}

} // namespace sched
