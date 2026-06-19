/**
 * @file ThreadPool.hpp
 * @brief Thread Pool implementation for executing asynchronous tasks.
 *
 * This file declares a fixed-size thread pool that maintains a queue of tasks.
 * Idle worker threads block on a condition variable until tasks are enqueued,
 * enabling concurrent execution of independent workloads.
 */

#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <functional>
#include <thread>
#include <vector>

using namespace std;

/**
 * @class ThreadPool
 * @brief A fixed-size pool of worker threads processing a task queue.
 *
 * Enables concurrent execution of incoming client request handles, minimizing
 * thread creation overhead. This class is non-copyable and non-assignable.
 */
class ThreadPool {
private:
  vector<thread> workers;                  ///< Active worker threads in the pool
  queue<std::function<void()>> taskQueue;  ///< Thread-safe FIFO queue of pending tasks
  std::mutex queueMtx;                     ///< Mutex protecting the taskQueue and stop state
  std::condition_variable cv;              ///< Condition variable to notify workers of new tasks
  std::atomic<bool> stop;                  ///< Flag signaling workers to stop and exit

  /**
   * @brief Loop executed by worker threads to retrieve and execute tasks.
   */
  void workerLoop();

public:
  /**
   * @brief Constructs the ThreadPool with a given thread count.
   * @param threads Number of worker threads to start. Defaults to hardware_concurrency() * 2.
   */
  ThreadPool(std::size_t threads = std::thread::hardware_concurrency() * 2);

  /**
   * @brief Destructor that joins and cleans up all worker threads.
   *
   * Signals all threads to stop, wakes up any waiting workers, and joins
   * each worker thread to ensure graceful shutdown.
   */
  ~ThreadPool();

  /**
   * @brief Enqueues a callable task for execution by the next available worker thread.
   * @param task A std::function wrapping the task to run.
   */
  void enqueueTask(std::function<void()> task);

  // Non-copyable and non-assignable to prevent accidental copying of active thread resources
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
};