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
  uint16_t PORT = 8080;                           // Listening port of the reverse proxy
  std::string ORIGIN;                             // Target upstream backend server URL
  int TTL = 60;                                   // Default cache entry lifetime (seconds)
  size_t SHARDS_COUNT = compute_default_shards(); // Number of cache shards to reduce lock contention
  size_t THREAD_COUNT = 150;                      // Worker thread pool size for concurrent client connections
  size_t CACHE_CAPACITY = 65536;                  // Maximum number of cached responses
  int CONNECT_TIMEOUT = 2;                        // Upstream connection timeout (seconds)
  int READ_TIMEOUT = 5;                           // Upstream read timeout (seconds)
  int WRITE_TIMEOUT = 2;                          // Upstream write timeout (seconds)
  int MAX_HEADER_BYTES = 8192;                    // Maximum request header buffer size (bytes)
  size_t MAX_CACHEABLE = 5 * 1024 * 1024;         // Maximum cacheable response body size (bytes)
  size_t MAX_REQUESTS_PER_CONNECTION = 100;       // Max requests per keep-alive client connection
  int KEEPALIVE_IDLE_TIMEOUT = 2;                 // Idle client connection timeout (seconds)

  static Config from_env();
};

extern Config config;
