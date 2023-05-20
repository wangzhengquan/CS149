#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <queue>
#include <map>
#include <memory>
#include <list>
#include <set>
#include <limits.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
     private:
        class ReadyBulk{
        public:
            //std::queue<Task> queue;
            TaskID bulk_id;
            IRunnable* runnable;
            int task_id{0};
            int num_total_tasks{0};
            std::mutex mutex;
            std::condition_variable cv;
            int num_tasks_completed{0};
            
            // std::atomic<int> num_tasks_completed{0}; 
            ReadyBulk(TaskID _bulk_id, IRunnable* _runnable, int _num_total_tasks):
             bulk_id(_bulk_id), runnable(_runnable), num_total_tasks(_num_total_tasks){}
        };


        class WaitBulk {
        public:
            TaskID bulk_id;
            IRunnable *runnable;
            int num_total_tasks;
            std::list<TaskID> deps{};
            WaitBulk( TaskID _bulk_id, IRunnable *_runnable, int _num_total_tasks, const std::vector<TaskID>& _deps) : 
                bulk_id(_bulk_id), runnable(_runnable), num_total_tasks(_num_total_tasks){
                for(TaskID dep : _deps){
                    deps.push_back(dep);
                }
            }

        };

        class FinishQueue{
            public:
            std::set<TaskID> queue{};
            std::mutex mutex;
        };

        class ReadyQueue{
            public:
            std::mutex mutex;
            std::condition_variable cv;
            std::queue<std::shared_ptr<ReadyBulk> > queue{};
            bool stop{false};
            
            std::mutex bulk_finished_mutex;
            std::condition_variable bulk_finished_cv;
            int num_bulks_complete{0};
            int num_total_bulks{0};

        };

        class WaitQueue{
            public:
            std::list<WaitBulk> queue;
            std::mutex mutex;
            std::condition_variable cv;
        };
       
      

        
        std::vector<std::thread> _threads_pool;

        TaskID _next_bulk_id{0};
        ReadyQueue ready_queue;
        WaitQueue wait_queue;
        FinishQueue finish_queue;

        int my_stopi = 0;

    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

#endif
