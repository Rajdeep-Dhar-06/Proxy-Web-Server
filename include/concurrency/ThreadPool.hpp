#pragma once
#include <functional>
#include <thread>
#include <vector>

#include "../structures/ThreadSafeQueue.hpp"

class ThreadPool {
 public:
  // Constructor and destructor
  ThreadPool(std::size_t threads);
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
  ThreadSafeQueue<std::function<void()>> taskQueue;
};