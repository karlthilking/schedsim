#ifndef SCHEDSIM_MLFQ_H
#define SCHEDSIM_MLFQ_H

namespace scheduler {
template<size_t N>  // N = number of queues
class mlfq {
private:
    std::array<std::queue<std::unique_ptr<task>>, N> tasks;
    std::vector<std::thread>                         threads;
    std::condition_variable                          cv;
    std::array<std::mutex, N>                        locks;
    milliseconds                                     timeslice;
    u32                                              ntasks;
    bool                                             stop;
    bool                                             pause;
    
    /*  
     *  Get the user and system CPU usage difference beetween
     *  the previous and current rusage struct in milliseconds;
     *  when diff >= timeslice the task should be demoted to a 
     *  lower queue level
     */
    u32
    get_ms_diff(const struct rusage &prev, const struct rusage &cur)
    {
        u32 prev_ms = (prev.ru_utime.tv_sec * 1000) + 
                      (prev.ru_utime.tv_usec / 1000) +
                      (prev.ru_stime.tv_sec * 1000) + 
                      (prev.ru_stime.tv_usec / 1000);
        u32 cur_ms = (cur.ru_utime.tv_sec * 1000) +
                     (cur.ru_utime.tv_usec / 1000) +
                     (cur.ru_stime.tv_sec * 1000) +
                     (cur.ru_stime.tv_usec / 1000);
        return cur_ms - prev_ms;
    }
    
    /*  
     *  Run the task for a timeslice and, unless the tasks exits after its
     *  timeslice, determine at which queue level to reinsert the task
     */
    void
    schedule(task *t, u32 level) noexcept
    {
        assert(t->get_state() != task_state::RUNNING ||
               t->get_state() != task_state::FINISHED);

        struct rusage prev_ru = t->ru;
        struct rusage cur_ru;
        if (t->get_state() == task_state::RUNNABLE)
            t->run();
        else if (t->get_state() == task_state::STOPPED)
            kill(t->get_pid(), SIGCONT);
        t->set_state(task_state::RUNNING);

        std::this_thread::sleep_for(timeslice);
        kill(t->get_pid(), SIGSTOP);

        int rc, wstat;
        if ((rc = wait4(t->get_pid(), &wstat, 0, &cur_ru)) < 0)
            err(EXIT_FAILURE, "wait4");
        else if (rc == 0)
            err(EXIT_FAILURE, "child state did not change");
        
        if (WIFEXITED(wstat)) {
            t->set_state(task_state::FINISHED);
            std::cout << t->get_pid() << " exited with exit code "
                      << WEXITSTATUS(wstat) << '\n';
        } else if (WIFSTOPPED(wstat)) {
            t->set_state(task_state::STOPPED);
            if (get_ms_diff(prev_ru, cur_ru) >= timeslice) {
                /* demote queue level and insert there */
                t->ru = cur_ru;
                enqueue(t, level - 1);
            } else {
                /* keep on the same queue level */
                enqueue(t, level);
            }
        }
        return;
    }

public:
    mlfq(u32 ncpus = 1, milliseconds timeslice = 24ms,
         seconds prio_boost_freq = 1s) noexcept
        : timeslice(timeslice), stop(false), ntasks(0)
    {
        threads.reserve(ncpus);
        for (u32 cpuid = 0; cpuid < ncpus; ++cpuid) {
            threads.emplace_back([this]{
                while (true) {
                    task *t;
                    u32 level;
                    for (level = 0; level < N; ++level) {
                        std::lock_guard<std::mutex> lock(locks[level]);
                        cv.wait(lock, [this]{
                            return (stop || ntasks > 0) && !pause;
                        });
                        if (tasks[level].empty()) {
                            if (stop && ntasks == 0)
                                return;
                            continue;
                        }
                        t = tasks[level].front().release();
                        tasks[level].pop();
                        --ntasks;
                        break;
                    }
                    schedule(t, level);
                }
            });
        }
        /* priority boost thread */
        threads.emplace_back([this]{
            do {
                std::this_thread::sleep_for(prio_boost_freq);
                pause = true;
                for (u32 level = 1; level < N; ++level) {
                    std::scoped_lock lock(locks[0], locks[level]);
                    while (!tasks[level].empty()) {
                        tasks[0].push(std::move(tasks[level].front()));
                        tasks[level].pop();
                    }
                }
                pause = false;
                cv.notify_all();
            } while (!stop);
        });
    }
    
    ~mlfq() noexcept
    {
        stop = true;
        cv.notify_all();
        for (std::thread &thrd : threads)
            thrd.join();
    }
    
    /*  
     *  By default, a newly created task is insterted into the highest
     *  priority queue (tasks[0]), but enqueue() is also called by a thread
     *  reinserting a previously running task so the enqueue functions allow
     *  for specifying a queue level; there is an exception for the argument
     *  list enqueue function which can assume the task is newly created
     */
    void
    enqueue(task *t, u32 level = 0) noexcept
    {
        std::lock_guard<std::mutex> lock(locks[level]);
        tasks[level].emplace(t);
        ++ntasks;
        cv.notify(locks[level]);
    }

    template<typename T>
    void
    enqueue(T &&t, u32 level = 0) noexcept
    {
        std::lock_guard<std::mutex> lock(locks[level]);
        tasks[level].push(std::make_unique<task>(std::forward<T>(task)));
        ++ntasks;
        cv.notify(locks[level]);
    }
    
    template<typename T, typename ...Args>
    void
    enqueue(Args &&...args) noexcept
    {
        std::lock_guard<std::mutex> lock(locks[0]);
        tasks[0].emplace(new T(std::foward<Args>(args)...));
        ++ntasks;
        cv.notify(locks[0]);
    }
};
} // namespace scheduler
