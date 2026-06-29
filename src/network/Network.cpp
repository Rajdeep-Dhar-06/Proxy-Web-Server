#include "network/Network.hpp"

#include <sockpp/tcp_acceptor.h>

#include <exception>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <cstdlib>
#include <iostream>
#include <memory>

#include "vendor/httplib.h"
#include "network/HTTPParser.hpp"

void run_server(uint16_t port, ThreadPool& pool, ShardedCache& cache, const std::string& origin, RequestCoalescer& coalescer) {
  sockpp::tcp_acceptor acceptor(port);  // socket() -> bind() -> listen()
  if (!acceptor) {
    std::cerr << "[ERROR] Failed to bind to port " << port << ": " << acceptor.last_error_str() << "\n";
    exit(EXIT_FAILURE);
  }
  std::cout << "[NETWORK] Listening on port " << port << "\n";

  while (true) {
    sockpp::tcp_socket client = acceptor.accept();  // Move-only connection handle
    if (!client) {
      std::cerr << "[WARNING] Accept failed: " << client.last_error_str() << "\n";
      continue;
    }

    // sockpp::tcp_socket is move-only, so we wrap it in a std::shared_ptr to
    // allow copying within the thread pool task closure.
    auto socket_ptr = std::make_shared<sockpp::tcp_socket>(std::move(client));
    pool.enqueueTask([socket_ptr, &cache, origin, &coalescer]() { handle_client(std::move(*socket_ptr), cache, origin, coalescer); });
  }
}

void handle_client(sockpp::tcp_socket client, ShardedCache& cache, const std::string& origin, RequestCoalescer& coalescer) {
  try {
    HttpRequest req = parse_request(client);
    std::string url = std::string(req.host) + std::string(req.path);

    // Primary cache lookup check.
    auto cached = cache.get(url);
    if (cached.has_value()) {
      std::cout << "[CACHE] HIT: " << req.method << " " << req.path << std::endl;
      std::string response = cached.value();
      inject_cache_header(response, "X-Cache: HIT\r\n");
      client.write_n(response.data(), response.size());
      return;
    }

    // Attempt to coalesce duplicate concurrent requests targeting the same URL.
    auto ticket = coalescer.start_or_join(url);
    if (ticket.is_owner) {
      // Double check in case another request populated the cache right before we claimed ownership.
      auto cached = cache.get(url);
      if (cached.has_value()) {
        coalescer.complete(url);
        std::cout << "[CACHE] HIT: " << req.method << " " << req.path << std::endl;
        std::string response = cached.value();
        inject_cache_header(response, "X-Cache: HIT\r\n");
        client.write_n(response.data(), response.size());
        return;
      }

      try {
        int ttl = 60; // Default TTL of 60 seconds
        std::string response = fetch_from_backend(origin, req.path, &ttl);

        // Cache the newly retrieved content if caching is enabled (ttl > 0)
        if (ttl > 0) {
          cache.put(url, response, ttl);
        }
        std::cout << "[CACHE] MISS: " << req.method << " " << req.path << std::endl;
        inject_cache_header(response, "X-Cache: MISS\r\n");

        // Unblock waiting sibling threads.
        coalescer.complete(url);
        client.write_n(response.data(), response.size());
      } catch (...) {
        // Broadcast the failure to waiting threads to prevent deadlock, then rethrow.
        coalescer.fail(url, std::current_exception());
        throw;
      }
    } else {
      // Waiter thread: block until the owner thread resolves the cache promise.
      try {
        ticket.done.get();
        auto cached = cache.get(url);
        if (!cached.has_value()) {
          throw ProxyException(500, "Internal Server Error", "Coalesced response not found in cache");
        }
        std::cout << "[CACHE] HIT (Coalesced): " << req.method << " " << req.path << std::endl;
        std::string response = cached.value();
        inject_cache_header(response, "X-Cache: HIT\r\n");
        client.write_n(response.data(), response.size());
        return;
      } catch (const ProxyException&) {
        throw;
      } catch (const std::exception& e) {
        throw ProxyException(502, "Bad Gateway", "Upstream request failed or owner thread crashed: " + std::string(e.what()));
      }
    }
  } catch (const SocketClosedException&) {
    // Silent return when the client socket is closed or read fails
    return;
  } catch (const ProxyException& e) {
    std::cerr << "[PROXY ERROR] " << e.what() << "\n";
    std::string err_resp = ProxyException::generate_http_response(e.get_status_code(), e.get_http_message());
    client.write_n(err_resp.data(), err_resp.size());
  } catch (const std::exception& e) {
    std::cerr << "[FATAL CRASH] " << e.what() << "\n";
    std::string err_resp = ProxyException::generate_http_response(500, "Internal Server Error");
    client.write_n(err_resp.data(), err_resp.size());
  }
}

std::string fetch_from_backend(const std::string& origin, const std::string& path, int* ttl) {
  // thread-local client persistent connection to backend origin
  thread_local std::unique_ptr<httplib::Client> backend = nullptr;
  // Configuration
  if (!backend) {
    backend = std::make_unique<httplib::Client>(origin);
    backend->set_keep_alive(true);
    backend->set_connection_timeout(5, 0);
    backend->set_read_timeout(10, 0);
    backend->set_write_timeout(5, 0);
    backend->set_follow_location(true);
    backend->enable_server_certificate_verification(false);
  }

  // Fetch using httplib
  auto res = backend->Get(path.c_str());
  if (!res) throw ProxyException(502, "Bad Gateway", "Backend unreachable");
  if (res->status != 200) throw ProxyException(res->status, "Upstream Error", "Failed");

  *ttl = 60; // default value
  if (res->has_header("Cache-Control")) {
    std::string cache_control = res->get_header_value("Cache-Control");

    if (cache_control.find("no-store") != std::string::npos ||
        cache_control.find("no-cache") != std::string::npos ||
        cache_control.find("private") != std::string::npos) {
      *ttl = 60; // Temporary caching of 60s
    } else {
      size_t pos = cache_control.find("max-age=");
      if (pos != std::string::npos) {
        std::string num_str = cache_control.substr(pos + 8);
        size_t comma_pos = num_str.find(',');
        if (comma_pos != std::string::npos) {
          num_str = num_str.substr(0, comma_pos);
        }
        try {
          *ttl = std::stoi(num_str);
        } catch (...) {
          *ttl = 60;  
        }
      }
    }
  }

  std::string full_response = "HTTP/1.1 200 OK\r\n";

  // Forward safe headers from the origin response. Recalculated Content-Length and attached later.
  for (const auto& header : res->headers) {
    if (header.first == "Content-Length" || header.first == "Transfer-Encoding") continue;
    full_response += header.first + ": " + header.second + "\r\n";
  }

  // Adding body
  full_response += "Content-Length: " + std::to_string(res->body.size()) + "\r\n\r\n";
  full_response += res->body;

  return full_response;
}