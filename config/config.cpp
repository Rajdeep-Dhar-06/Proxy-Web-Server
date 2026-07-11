#include "config.hpp"

#include <cstdlib>

Config Config::from_env() {
  Config cfg;
  if (const char* port_env = std::getenv("PORT")) {
    cfg.PORT = static_cast<uint16_t>(std::stoi(port_env));
  }
  if (const char* origin_env = std::getenv("ORIGIN")) {
    cfg.ORIGIN = origin_env;
  }
  if (const char* ttl_env = std::getenv("TTL")) {
    cfg.TTL = std::stoi(ttl_env);
  }
  if (const char* shards_env = std::getenv("SHARDS_COUNT")) {
    cfg.SHARDS_COUNT = std::stoi(shards_env);
  }
  if (const char* threads_env = std::getenv("THREAD_COUNT")) {
    cfg.THREAD_COUNT = std::stoi(threads_env);
  }
  if (const char* cap_env = std::getenv("CACHE_CAPACITY")) {
    cfg.CACHE_CAPACITY = std::stoi(cap_env);
  }
  if (const char* conn_to_env = std::getenv("CONNECT_TIMEOUT")) {
    cfg.CONNECT_TIMEOUT = std::stoi(conn_to_env);
  }
  if (const char* read_to_env = std::getenv("READ_TIMEOUT")) {
    cfg.READ_TIMEOUT = std::stoi(read_to_env);
  }
  if (const char* write_to_env = std::getenv("WRITE_TIMEOUT")) {
    cfg.WRITE_TIMEOUT = std::stoi(write_to_env);
  }
  if (const char* max_hdr_env = std::getenv("MAX_HEADER_BYTES")) {
    cfg.MAX_HEADER_BYTES = std::stoi(max_hdr_env);
  }
  if (const char* max_cacheable_env = std::getenv("MAX_CACHEABLE")) {
    cfg.MAX_CACHEABLE = std::stoull(max_cacheable_env);
  }
  if (const char* max_req_conn_env = std::getenv("MAX_REQUESTS_PER_CONNECTION")) {
    cfg.MAX_REQUESTS_PER_CONNECTION = std::stoi(max_req_conn_env);
  }
  if (const char* idle_to_env = std::getenv("KEEPALIVE_IDLE_TIMEOUT")) {
    cfg.KEEPALIVE_IDLE_TIMEOUT = std::stoi(idle_to_env);
  }
  return cfg;
}

// Global configuration instance
Config config = Config::from_env();