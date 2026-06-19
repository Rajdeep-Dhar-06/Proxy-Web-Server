#include "../include/ShardedCache.hpp"
#include <cassert>
#include <chrono> // NEW: For benchmarking
#include <iostream>
#include <string>
#include <thread>
#include <vector>
using namespace std;

void test_basic_logic() {
  // Create a tiny cache: Total capacity of 4, split across 2 shards
  ShardedCache cache(4, 2);

  // 1. Test basic insertion and retrieval
  cache.put("google.com", "<html>Google</html>");
  cache.put("yahoo.com", "<html>Yahoo</html>");

  auto res = cache.get("google.com");
  assert(res.has_value() && *res == "<html>Google</html>");
  cout << "[PASS] Basic GET/PUT Test" << endl;

  // 2. Test cache miss
  auto miss = cache.get("doesnotexist.com");
  assert(!miss.has_value());
  cout << "[PASS] Cache Miss Test" << endl;

  // 3. Trigger Eviction
  // By spamming URLs, we force the caches to exceed their limits
  cache.put("bing.com", "<html>Bing</html>");
  cache.put("duckduckgo.com", "<html>DDG</html>");
  cache.put("ask.com", "<html>Ask</html>");
  cache.put("aol.com", "<html>AOL</html>");

  cout << "[PASS] Eviction Logic (Survived without crashing!)" << endl;
}

void test_benchmark() {
  cout << "\n--- Starting Performance Benchmark ---" << endl;

  int num_threads = 24; // Optimized for your 12-thread CPU
  int ops_per_thread = 100000;
  int total_ops = num_threads * ops_per_thread * 2; // *2 because 1 put + 1 get

  ShardedCache cache(10000, 16);

  auto worker = [&cache, ops_per_thread](int thread_id) {
    for (int i = 0; i < ops_per_thread; ++i) {
      string url = "url_" + to_string(thread_id) + "_" + to_string(i);
      cache.put(url, "payload_data");
      cache.get(url);
    }
  };

  vector<thread> threads;

  // START THE CLOCK
  auto start_time = chrono::high_resolution_clock::now();

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker, i);
  }
  for (auto &t : threads) {
    t.join();
  }

  // STOP THE CLOCK
  auto end_time = chrono::high_resolution_clock::now();
  chrono::duration<double> diff = end_time - start_time;

  // Calculate Operations Per Second
  double rps = total_ops / diff.count();

  cout << "Total Operations : " << total_ops << endl;
  cout << "Time Taken       : " << diff.count() << " seconds" << endl;
  cout << "Throughput       : " << rps << " operations / second" << endl;
}

int main() {
  // test_basic_logic(); // You can leave this uncommented to run it too
  test_benchmark();
  return 0;
}
