#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <functional>
#include <thread>
#include <vector>

using namespace std;

class ThreadPool {
private:
  vector<thread> workers;
  queue<std::function<void()>> taskQueue;
  std::mutex queueMtx;
  std::condition_variable cv;
  std::atomic<bool> stop;
  void workerLoop();

public:
  ThreadPool(std::size_t threads = std::thread::hardware_concurrency() * 2);
  ~ThreadPool();
  void enqueueTask(std::function<void()> task);
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
};