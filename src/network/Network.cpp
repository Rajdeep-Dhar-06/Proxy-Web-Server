#include "network/Network.hpp"

#include <sockpp/tcp_acceptor.h>

#include <memory>

#include "cache/ShardedCache.hpp"
#include "concurrency/ThreadPool.hpp"
#include "config/config.hpp"
#include "logger/Logger.hpp"
#include "middleware/CacheHandler.hpp"
#include "middleware/ParseHandler.hpp"
#include "middleware/UpstreamHandler.hpp"
#include "network/HttpContext.hpp"
#include "network/Pipeline.hpp"
#include "network/RequestCoalescer.hpp"

ProxyServer::ProxyServer(uint16_t& port) : port(port), thread_pool(config.THREAD_COUNT) {
  cache = std::make_shared<ShardedCache>(config.CACHE_CAPACITY, config.SHARDS_COUNT);
  coalescer = std::make_shared<RequestCoalescer>();
  pipeline = std::make_shared<Pipeline>();
  pipeline->add_handler(std::make_shared<ParseHandler>());
  pipeline->add_handler(std::make_shared<CacheHandler>(cache, coalescer));
  pipeline->add_handler(std::make_shared<UpstreamHandler>(config.ORIGIN));
}

void ProxyServer::run_server() {
  sockpp::tcp_acceptor acceptor(port);  // socket() -> bind() -> listen()
  if (!acceptor) {
    Logger::get_instance().log("Failed to bind to port " + std::to_string(port) + ": " + acceptor.last_error_str(), LoggerLevel::ERROR);
    exit(EXIT_FAILURE);
  }
  Logger::get_instance().log("Listening on port " + std::to_string(port), LoggerLevel::INFO);

  while (true) {
    // Move-only connection handle as its a OS fd
    sockpp::tcp_socket client = acceptor.accept();
    if (!client) {
      Logger::get_instance().log("Accept failed: " + client.last_error_str(), LoggerLevel::WARNING);
      continue;
    }

    // Moves the socket allocation from stack to heap to survive the async task
    auto socket_ptr = std::make_shared<sockpp::tcp_socket>(std::move(client));
    thread_pool.enqueue_task([socket_ptr, pipeline = pipeline]() {
      try {
        HttpContext ctx(*socket_ptr);
        pipeline->execute(ctx);
      } catch (const std::exception& e) {
        Logger::get_instance().log("Exception during client processing: " + std::string(e.what()), LoggerLevel::ERROR);
      }
    });
  }
}