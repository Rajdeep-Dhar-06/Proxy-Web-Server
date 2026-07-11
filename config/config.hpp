#pragma once
#include <cstdint>
#include <string>
#include <thread>

inline unsigned compute_thread_count() {
  unsigned hc = std::thread::hardware_concurrency();
  if (hc == 0) hc = 4;
  return hc * 2;
}

struct Config {
  uint16_t PORT = 8080;
  std::string ORIGIN;
  int TTL = 60;  // seconds
  int SHARDS_COUNT = compute_thread_count() / 2;
  int THREAD_COUNT = compute_thread_count();
  int CACHE_CAPACITY = 65536;
  int CONNECT_TIMEOUT = 2;
  int READ_TIMEOUT = 5; // seconds
  int WRITE_TIMEOUT = 2;
  int MAX_HEADER_BYTES = 8192;
  size_t MAX_CACHEABLE = 5 * 1024 * 1024;  // 5MB default

  static Config from_env();
};

extern Config config;
