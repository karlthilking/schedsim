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

void sjf::schedule(task &curr_task) noexcept
{
    if (!(curr_task.get_state() == task_state::RUNNABLE)) {
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
    : t_curr(0), done(false)
{
    std::thread scheduler([this] {
        while (true) {
            task curr_task;
            {
                std::unique_lock<std::mutex> lock(tasks_mutex);
                cv.wait(lock, [this] { 
                    return this->done || !(this->tasks.empty());
                });
                if (this->done && this->tasks.empty()) {
                    return;
                }
                curr_task = this->tasks.top();
                this->tasks.pop();
            }
            this->schedule(curr_task);
        }
    });
    scheduler.join();
}

sjf::sjf(const std::vector<task> &tasks_)
    : tasks(begin(tasks_), end(tasks_)), t_curr(0), done(false)
{
    u32 n_imm_tasks = tasks.size();
    std::thread scheduler([&] {
        while (n_imm_tasks--) {
            task curr_task;
            {
                std::scoped_lock lock(this->tasks_mutex);
                if (this->done && this->tasks.empty()) {
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
                    return this->done | !(this->tasks.empty());
                });
                if (this->done && this->tasks.empty()) { 
                    return; 
                }
                curr_task = this->tasks.top();
                this->tasks.pop();
            }
            this->schedule(curr_task);
        }
    });
    scheduler.join();
}

void sjf::enqueue(task &new_task) noexcept 
{
    std::scoped_lock lock(this->tasks_mutex);
    if (this->t_curr <= new_task.get_t_arr() && 
        new_task.get_state() != task_state::RUNNABLE) {
        new_task.set_state(task_state::RUNNABLE);
    } else if (new_task.get_state() != task_state::NOTREADY) {
        new_task.set_state(task_state::NOTREADY);
    }
    this->tasks.push(new_task);
    this->cv.notify_one();
}

void sjf::stop() noexcept 
{ 
    done = true; 
    this->cv.notify_one();
}

void sjf::display_stats() const noexcept
{
    float avg_turnaround, avg_response;
    int task_no = 1;
    for (const auto &[t_turnaround, t_response] : this->stats) {
        std::cout << task_no++ << "Turnaround Time: " << t_turnaround 
                  << ", Response Time: " << t_response << std::endl;
        avg_turnaround += t_turnaround;
        avg_response += t_response;
    }
    avg_turnaround /= this->stats.size();
    avg_response /= this->stats.size();
    std::cout << "Average Turnaround Time: " << avg_turnaround
              << ", Average Response Time: " << avg_response << std::endl;
}


