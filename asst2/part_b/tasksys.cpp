#include "tasksys.h"
#include <iostream>
#include <chrono>
#include <unistd.h>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
  
    _threads_pool.reserve(num_threads);
    
    for (int i = 0; i < num_threads; i++) {
        _threads_pool.emplace_back([&]() {
            while (true) {
                std::unique_lock<std::mutex> ready_queue_lock(ready_queue.mutex);
                if(ready_queue.stop){
                    // std::cout << "stop "<< my_stopi << std::endl;
                    my_stopi++;
                    ready_queue_lock.unlock();
                    break;
                }
               
                if(ready_queue.queue.empty()){
                    ready_queue.cv.wait(ready_queue_lock);
                    ready_queue_lock.unlock();
                }
                else{
                    std::shared_ptr<ReadyBulk> ready_bulk = ready_queue.queue.front();
                    int task_id = ready_bulk->task_id++;
                    if(task_id >= ready_bulk->num_total_tasks) {
                        ready_queue.queue.pop();
                        ready_queue_lock.unlock();
                    } else {
                        ready_queue_lock.unlock();
                        ready_bulk->runnable->runTask(task_id, ready_bulk->num_total_tasks);

                        std::unique_lock<std::mutex> ready_bulk_lock(ready_bulk->mutex);
                        ready_bulk->num_tasks_completed++;
                        if(ready_bulk->num_tasks_completed==ready_bulk->num_total_tasks){
                            ready_bulk_lock.unlock();
                            
                            std::unique_lock<std::mutex> ready_queue_bulk_finished_lock(ready_queue.bulk_finished_mutex);
                            ready_queue.num_bulks_complete++;
                            if(ready_queue.num_bulks_complete == ready_queue.num_total_bulks){
                                ready_queue_bulk_finished_lock.unlock();
                                ready_queue.bulk_finished_cv.notify_one();
                            } else {
                                ready_queue_bulk_finished_lock.unlock();
                            }
                            
                            std::unique_lock<std::mutex> finish_queue_lock(finish_queue.mutex);
                            finish_queue.queue.insert(ready_bulk->bulk_id);
                            finish_queue_lock.unlock();

                            // finished bulk tasks, search the bulk  which has dependence with this bulk
                            std::unique_lock<std::mutex> wait_queue_lock(wait_queue.mutex);
                            for (auto wait_queue_it = wait_queue.queue.begin(); wait_queue_it != wait_queue.queue.end();){
                                WaitBulk & wait_bulk = *wait_queue_it;
                                for (auto it = wait_bulk.deps.begin(); it != wait_bulk.deps.end();)
                                {
                                     if (*it == ready_bulk->bulk_id){
                                         it = wait_bulk.deps.erase(it);
                                         break;
                                     } else
                                         ++it;
                                }

                                if(wait_bulk.deps.empty()){
                                    ready_queue_lock.lock();
                                    ready_queue.queue.push(std::shared_ptr<ReadyBulk>(new ReadyBulk{wait_bulk.bulk_id, wait_bulk.runnable, wait_bulk.num_total_tasks}));
                                    ready_queue_lock.unlock();
                                    ready_queue.cv.notify_all();
                                    wait_queue_it = wait_queue.queue.erase(wait_queue_it);

                                } else {
                                    ++wait_queue_it;
                                }
                                
                            }

                            wait_queue_lock.unlock();
                        } else {
                            ready_bulk_lock.unlock();
                        } 
                    }

                } 
            }
        });
    } 
   
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    // std::cout << "=========~TaskSystemParallelThreadPoolSleeping"  << std::endl ;
    sync();
    std::unique_lock<std::mutex> lock(ready_queue.mutex);
    ready_queue.stop = true;
    lock.unlock();
    ready_queue.cv.notify_all();
    for (auto& thread : _threads_pool)
        thread.join();
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    
    const std::vector<TaskID> deps;
    runAsyncWithDeps(runnable, num_total_tasks, deps);
    
    sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {
    
    ready_queue.num_total_bulks++;
    
    std::vector<TaskID> newdeps;
    std::unique_lock<std::mutex> finish_queue_lock(finish_queue.mutex);
    for (TaskID dep : deps) {
        if(finish_queue.queue.find(dep)==finish_queue.queue.end()){
            newdeps.push_back(dep);
        }
    }

    if(newdeps.empty()){
        finish_queue_lock.unlock();
        std::unique_lock<std::mutex> lock(ready_queue.mutex);
        TaskID bulk_id = _next_bulk_id++;
        ready_queue.queue.push(std::shared_ptr<ReadyBulk>(new ReadyBulk{bulk_id, runnable, num_total_tasks}));
        lock.unlock();
        ready_queue.cv.notify_all();
        return bulk_id;
    } else {
        std::unique_lock<std::mutex> lock(wait_queue.mutex);
        TaskID bulk_id = _next_bulk_id++;
        wait_queue.queue.push_back( WaitBulk{bulk_id, runnable, num_total_tasks, newdeps});
        
        finish_queue_lock.unlock();
        lock.unlock();
        return bulk_id;
    }
}

void TaskSystemParallelThreadPoolSleeping::sync() {
    std::unique_lock<std::mutex> lock(ready_queue.bulk_finished_mutex);
    // std::cout << "=========sync"  << std::endl ;
    while( ready_queue.num_bulks_complete < ready_queue.num_total_bulks){
        // std::cout << "wait before ready_queue.num_bulks_complete="  <<  ready_queue.num_bulks_complete  << ", num_total_bulks= " << ready_queue.num_total_bulks << std::endl;   
        ready_queue.bulk_finished_cv.wait(lock);
        // std::cout << "wait after bulks.num_bulks_complete="  <<  bulks.num_bulks_complete  << ", num_total_bulks= " << bulks.num_total_bulks << std::endl;   
    }
    lock.unlock();
    return;
}