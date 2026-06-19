#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t threads) {
  stop = false;
  for (size_t i = 0; i < threads; i++) {
    workers.emplace_back(&ThreadPool::workerLoop, this);
  }
}

void ThreadPool::enqueueTask(std::function<void()> task) {
  {
    std::unique_lock<std::mutex> lock(queueMtx);
    if (stop) return;
    taskQueue.push(std::move(task));
  }
  cv.notify_one();
}

void ThreadPool::workerLoop() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(queueMtx);
      cv.wait(lock, [this]() { return stop || !taskQueue.empty(); });

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
    stop = true;
    cv.notify_all();
    for (thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}