#include "concurrency/ThreadPool.hpp"

ThreadPool::ThreadPool(size_t threads) {
  for (size_t i = 0; i < threads; i++) {
    workers.emplace_back(&ThreadPool::worker_loop, this);
  }
}

void ThreadPool::enqueue_task(std::function<void()> task) { taskQueue.push(std::move(task)); }

void ThreadPool::worker_loop() {
  while (true) {
    auto task = taskQueue.pop();

    if (!task.has_value()) {
      return;
    }

    (*task)();
  }
}

ThreadPool::~ThreadPool() {
  taskQueue.stop();

  for (auto& worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}