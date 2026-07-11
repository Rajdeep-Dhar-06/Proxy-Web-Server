#include "concurrency/ThreadPool.hpp"

ThreadPool::ThreadPool(size_t threads) {
  stop = false;
  // Initialize worker threads, binding them to the internal workerLoop handler
  for (size_t i = 0; i < threads; i++) {
    workers.emplace_back(&ThreadPool::worker_loop, this);
  }
}

void ThreadPool::enqueue_task(std::function<void()> task) {
  {
    std::lock_guard<std::mutex> lock(queueMtx);
    // Ignore tasks if the thread pool is in shutdown mode
    if (stop) return;
    taskQueue.push(std::move(task));
  }
  // Wake up one worker thread to process the newly queued task
  cv.notify_one();
}

void ThreadPool::worker_loop() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(queueMtx);
      // Wait until a new task is available or the pool is stopped
      cv.wait(lock, [this]() { return stop || !taskQueue.empty(); });

      // Exit the loop if the pool is stopping and all remaining tasks have been completed
      if (stop && taskQueue.empty()) {
        return;
      }

      task = std::move(taskQueue.front());
      taskQueue.pop();
    }
    if (task) {
      task();
    }
  }
}

ThreadPool::~ThreadPool() {
  // Set the stop flag and notify all threads to break out of their wait loops
  {
    std::lock_guard<std::mutex> lock(queueMtx);
    stop = true;
  }
  cv.notify_all();

  // Gracefully join and wait for all active threads to complete their execution
  for (auto& worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}