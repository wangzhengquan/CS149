#include "tasksys.h"
#include <iostream>
#include <chrono>
#include <unistd.h>


IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads)  {}
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
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //

}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::thread* threads = new std::thread[num_total_tasks];
    for (int i = 0; i < num_total_tasks; i++) {
        threads[i] = std::thread(&IRunnable::runTask, runnable, i, num_total_tasks);
        
    }
    for (int i = 0; i < num_total_tasks; i++) {
        threads[i].join();
    }
    delete [] threads;
    
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    return 0;
}

void TaskSystemParallelSpawn::sync() {
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

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads):  ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    _threads_pool.reserve(num_threads);
    
    for (int i = 0; i < num_threads; i++) {
        _threads_pool.emplace_back([&]() {
            while (true) {
                std::unique_lock<std::mutex> lock(tasks.mutex);
                if(tasks.stop){
                    break;
                }
                // std::cout << "wait after "<< tasks.num_tasks_completed  << std::endl;
                if(tasks.queue.empty()){
                    tasks.cv.wait(lock);
                    lock.unlock();
                }
                else{
                    Task task = tasks.queue.front();
                    tasks.queue.pop();
                    lock.unlock();
                    task.runnable->runTask(task.task_id, task.num_total_tasks);
                    tasks.num_tasks_completed++;
                    if(tasks.num_tasks_completed==tasks.num_total_tasks){
                        std::unique_lock<std::mutex> done_lock(tasks.done_mutex);
                        done_lock.unlock();
                        tasks.done_cv.notify_one();
                        
                    }  
                } 
                
               // std::cout << tasks.num_tasks_completed << ", " << task.num_total_tasks << std::endl;
            }
        });
    } 
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    std::unique_lock<std::mutex> lock(tasks.mutex);
    tasks.stop = true;
    lock.unlock();
    tasks.cv.notify_all();
    for (auto& thread : _threads_pool)
        thread.join();
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    
    // std::cout << "run num_total_tasks=" << num_total_tasks << std::endl;
    if(num_total_tasks == 0)
        return;
    std::unique_lock<std::mutex> done_lock(tasks.done_mutex); 
    std::unique_lock<std::mutex> lock(tasks.mutex);
    for (int i = 0; i < num_total_tasks; i++) {
        tasks.queue.push({runnable, i, num_total_tasks});
    }   
    // tasks.done = false;
    tasks.num_tasks_completed = 0;
    tasks.num_total_tasks = num_total_tasks; 
    lock.unlock(); 

    tasks.cv.notify_all();  
    tasks.done_cv.wait(done_lock);
    done_lock.unlock();
    
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
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
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //  

    _threads_pool.reserve(num_threads);
    
    for (int i = 0; i < num_threads; i++) {
        _threads_pool.emplace_back([&]() {
            while (true) {
                
                std::unique_lock<std::mutex> lock(tasks.mutex);
                if(tasks.stop){
                    break;
                }
                // std::cout << "wait after "<< tasks.num_tasks_completed  << std::endl;
                if(tasks.queue.empty()){
                    tasks.cv.wait(lock);
                    lock.unlock();
                }
                else{
                    Task task = tasks.queue.front();
                    tasks.queue.pop();
                    lock.unlock();
                    task.runnable->runTask(task.task_id, task.num_total_tasks);
                    tasks.num_tasks_completed++;
                    if(tasks.num_tasks_completed==tasks.num_total_tasks){
                        // this lock is necessary to ensure the main thread has entry into wait
                        std::unique_lock<std::mutex> done_lock(tasks.done_mutex);
                        done_lock.unlock();
                        tasks.done_cv.notify_one();
                        
                    }  
                } 
                
               // std::cout << tasks.num_tasks_completed << ", " << task.num_total_tasks << std::endl;
            }
        });
    } 
   
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    std::unique_lock<std::mutex> lock(tasks.mutex);
    tasks.stop = true;
    lock.unlock();
    tasks.cv.notify_all();
    for (auto& thread : _threads_pool)
        thread.join();
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
   // std::cout << "run num_total_tasks=" << num_total_tasks << std::endl;
    if(num_total_tasks == 0)
        return;
    std::unique_lock<std::mutex> done_lock(tasks.done_mutex); 
    std::unique_lock<std::mutex> lock(tasks.mutex);
    for (int i = 0; i < num_total_tasks; i++) {
        tasks.queue.push({runnable, i, num_total_tasks});
    }   
    // tasks.done = false;
    tasks.num_tasks_completed = 0;
    tasks.num_total_tasks = num_total_tasks; 
    lock.unlock(); 

    tasks.cv.notify_all();  
    tasks.done_cv.wait(done_lock);
    done_lock.unlock();

    
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
