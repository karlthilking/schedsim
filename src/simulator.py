import numpy as np
import copy
from enum import Enum

class task_state(Enum):
    ready       = 0
    running     = 1
    blocked     = 2
    finished    = 3

class task:
    def __init__(self, t_arrival, rt_total):
        self.t_arrival      = t_arrival         # arrival time
        self.t_firstrun     = -1                # first time scheduled
        self.t_completion   = -1                # completion time
        self.rt_current     = 0                 # current runtime
        self.rt_total       = rt_total          # total runtime
        self.state          = task_state.ready  # task state
        self.t_turnaround   = -1
        self.t_response     = -1
    
    def __eq__(self, other):
        if isinstance(other, task):
            return self.t_firstrun == other.t_firstrun
        return False
    
    # t_turnaround = t_completion - t_arrival
    def complete(self, t_cur):
        self.t_completion = t_cur
        self.t_turnaround = self.t_completion - self.t_arrival
    
    def respond(self, t_cur):
        self.t_firstrun = t_cur
        self.t_response = self.t_firstrun - self.t_arrival

    
# Shortest-Job First Scheduler
class sjf_sched:
    def __init__(self, tasks = None):
        self.tasks = tasks

    def set_tasks(self, tasks):
        self.tasks = tasks
    
    def get_shortest_job(self, t_cur):
        rt_min, sj = float('inf'), None
        for t in self.tasks:
            if t.t_arrival > t_cur or t.state == task_state.finished:
                continue
            if t.rt_total < rt_min:
                rt_min = t.rt_total
                sj = t
        return sj
    
    def all_tasks_done(self):
        return all(t.state == task_state.finished for t in self.tasks)

    def run(self):
        t_cur = 0
        t_total = np.sum([t.rt_total for t in self.tasks])
        while not self.all_tasks_done():
            cur_task = self.get_shortest_job(t_cur)
            cur_task.respond(t_cur)
            t_cur += cur_task.rt_total
            cur_task.rt_current += cur_task.rt_total
            cur_task.complete(t_cur)
            cur_task.state = task_state.finished
    
    def print_stats(self):
        avg_turnaround = np.mean([t.t_turnaround for t in self.tasks])
        avg_response = np.mean([t.t_response for t in self.tasks])
        print('SJF Scheduler Stats')
        print(f'Average Turnaround Time: {avg_turnaround}')
        print(f'Average Response Time: {avg_response}')

# Short Time-To-Completion-First Scheduler
class stcf_sched:
    def __init__(self, tasks = None, timeslice = 1):
        self.tasks = tasks
        self.timeslice = timeslice

    def set_tasks(self, tasks):
        self.tasks = tasks
    
    # return the job with shortest-time-to-completion
    def get_shortest_tc(self, t_cur):
        min_tc, stcj = float('inf'), None
        for t in self.tasks:
            if t.t_arrival > t_cur or t.state == task_state.finished:
                continue
            if t.rt_total - t.rt_current < min_tc:
                min_tc = t.rt_total - t.rt_current
                stcj = t
        return stcj
    
    def all_tasks_done(self):
        return all(t.state == task_state.finished for t in self.tasks)

    def run(self):
        t_cur = 0
        t_total = np.sum([t.rt_total for t in self.tasks])
        cur_task = None
        while not self.all_tasks_done():
            new_cur_task = self.get_shortest_tc(t_cur)
            # context switch
            if not cur_task is None and new_cur_task != cur_task:
                if cur_task.state != task_state.finished:
                    cur_task.state = task_state.blocked
            cur_task = new_cur_task
            if cur_task.state == task_state.ready:
                cur_task.state = task_state.running
                cur_task.respond(t_cur)
            t_increment = self.timeslice
            if cur_task.rt_total - cur_task.rt_current < self.timeslice:
                t_increment = cur_task.rt_total - cur_task.rt_current
            cur_task.rt_current += t_increment
            t_cur += t_increment
            if cur_task.rt_current >= cur_task.rt_total:
                cur_task.state = task_state.finished
                cur_task.complete(t_cur)

    def print_stats(self):
        avg_turnaround = np.mean([t.t_turnaround for t in self.tasks])
        avg_response = np.mean([t.t_response for t in self.tasks])
        print('STCF Scheduler Stats')
        print(f'Average Turnaround Time: {avg_turnaround}')
        print(f'Average Response Time: {avg_response}')

# First-In, First-Out Scheduler
class fifo_sched:
    def __init__(self, tasks = None):
        self.tasks = tasks
    
    def set_tasks(self, tasks):
        self.tasks = tasks

    def all_tasks_done(self):
        return all(t.state == task_state.finished for t in self.tasks)

    def run(self):
        ix_cur = 0
        t_cur = 0
        t_total = np.sum([t.rt_total for t in self.tasks])
        while not self.all_tasks_done():
            cur_task = self.tasks[ix_cur]
            cur_task.respond(t_cur)
            t_cur += cur_task.rt_total
            cur_task.rt_current += cur_task.rt_total
            cur_task.complete(t_cur)
            cur_task.state = task_state.finished
            ix_cur += 1
    
    def print_stats(self):
        avg_turnaround = np.mean([t.t_turnaround for t in self.tasks])
        avg_response = np.mean([t.t_response for t in self.tasks])
        print('FIFO Scheduler Stats:')
        print(f'Average Turnaround Time: {avg_turnaround}')
        print(f'Average Reponse Time: {avg_response}')


num_tasks = 10
tasks = [task(0, np.random.randint(0,100)) for i in range(num_tasks)]
schedulers = [
    sjf_sched(copy.deepcopy(tasks)),
    stcf_sched(copy.deepcopy(tasks), 16),
    fifo_sched(copy.deepcopy(tasks))
]
for s in schedulers:
    s.run()
    s.print_stats()

