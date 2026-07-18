#pragma once
#include <sockpp/tcp_socket.h>

#include <cstdint>
#include <memory>

#include "../concurrency/ThreadPool.hpp"
#include "./Pipeline.hpp"
#include "cache/ShardedCache.hpp"
#include "network/RequestCoalescer.hpp"

class ProxyServer {
 public:
  // Constructor and destructor
  ProxyServer(uint16_t& server_port);
  ~ProxyServer() = default;

  // Delete copy operations
  ProxyServer(const ProxyServer&) = delete;

  // Server management
  void run_server();

 private:
  // Members
  ThreadPool thread_pool;
  std::shared_ptr<ShardedCache> cache;
  std::shared_ptr<RequestCoalescer> coalescer;
  std::shared_ptr<Pipeline> pipeline;
  uint16_t port;
};