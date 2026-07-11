#pragma once
#include <cstdint>
#include <string>
#include <thread>

inline unsigned compute_default_shards() {
  unsigned hc = std::thread::hardware_concurrency();
  if (hc == 0) hc = 4;
  return hc;
}

struct Config {
  uint16_t PORT = 8080;
  std::string ORIGIN;
  int TTL = 60;  // seconds
  int SHARDS_COUNT = compute_default_shards();
  int THREAD_COUNT = 150;  // Default large pool for blocking I/O to handle concurrent keep-alive connections
  int CACHE_CAPACITY = 65536;
  int CONNECT_TIMEOUT = 2;
  int READ_TIMEOUT = 5; // seconds
  int WRITE_TIMEOUT = 2;
  int MAX_HEADER_BYTES = 8192;
  size_t MAX_CACHEABLE = 5 * 1024 * 1024;  // 5MB default
  int MAX_REQUESTS_PER_CONNECTION = 100;
  int KEEPALIVE_IDLE_TIMEOUT = 2;  // seconds (reduced from 10 to release idle connections quickly)

  static Config from_env();
};

extern Config config;

