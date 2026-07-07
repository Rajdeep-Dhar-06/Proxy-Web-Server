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
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
 public:
  // Constructor and destructor
  ThreadPool(std::size_t threads = std::thread::hardware_concurrency() * 2);
  ~ThreadPool();

  // Task management
  void enqueue_task(std::function<void()> task);

  // Delete copy operations
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

 private:
  // Helpers
  void worker_loop();

  // Members
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> taskQueue;
  std::mutex queueMtx;
  std::condition_variable cv;
  std::atomic<bool> stop;
};