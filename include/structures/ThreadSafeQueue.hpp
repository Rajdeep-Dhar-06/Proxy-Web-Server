#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class ThreadSafeQueue {
 public:
  // Queue Operations
  bool push(T item) {
    {
      std::lock_guard<std::mutex> lock(mtx);
      if (is_stopped) {
        return false;
      }
      task_queue.push(std::move(item));
    }
    cv.notify_one();
    return true;
  }

  std::optional<T> pop() {
    std::unique_lock<std::mutex> lock(mtx);
    while (task_queue.empty() && !is_stopped) {
      cv.wait(lock);
    }

    if (task_queue.empty() && is_stopped) {
      return std::nullopt;
    }

    T item = std::move(task_queue.front());
    task_queue.pop();
    return item;
  }

  void stop() {
    {
      std::lock_guard<std::mutex> lock(mtx);
      is_stopped = true;
    }
    cv.notify_all();
  }

 private:
  // Members
  std::queue<T> task_queue;
  std::mutex mtx;
  std::condition_variable cv;
  bool is_stopped{false};
};