#include "Network.hpp"

#include <sockpp/tcp_acceptor.h>
#include <exception>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <cstdlib>
#include <iostream>
#include <memory>

#include "../vendor/httplib.h"
#include "HTTPParser.hpp"

struct OriginInfo {
  std::string host;
  uint16_t port = 80;
};

OriginInfo parse_origin(const std::string& origin) {
  OriginInfo info;
  std::string url = origin;
  const std::string prefix = "http://";
  if (url.substr(0, prefix.size()) == prefix) {
    url = url.substr(prefix.size());
  }
  auto slash = url.find('/');
  if (slash != std::string::npos) {
    url = url.substr(0, slash);
  }
  auto colon = url.find(':');
  if (colon != std::string::npos) {
    info.host = url.substr(0, colon);
    info.port = static_cast<uint16_t>(std::stoi(url.substr(colon + 1)));
  } else {
    info.host = url;
    info.port = 80;
  }
  return info;
}

void run_server(uint16_t port, ThreadPool& pool, ShardedCache& cache, const string& origin, RequestCoalescer& coalescer) {
  sockpp::tcp_acceptor acceptor(port);  // socket() -> bind() -> listen()
  if (!acceptor) {
    std::cerr << "[ERROR] Failed to bind to port " << port << ": " << acceptor.last_error_str() << "\n";
    exit(EXIT_FAILURE);
  }
  std::cout << "[NETWORK] Listening on port " << port << "\n";

  while (true) {
    sockpp::tcp_socket client = acceptor.accept();  // move-only
    if (!client) {
      std::cerr << "[WARNING] Accept failed: " << client.last_error_str() << "\n";
      continue;
    }
    auto ptr = std::make_shared<sockpp::tcp_socket>(std::move(client));  // copy-able + move-only
    pool.enqueueTask([ptr, &cache, origin, &coalescer]() { handle_client(std::move(*ptr), cache, origin, coalescer); });
  }
}

void handle_client(sockpp::tcp_socket client, ShardedCache& cache, const string& origin, RequestCoalescer& coalescer) {
  char buffer[8192];

  auto read_res = client.read(buffer, sizeof(buffer) - 1);
  if (read_res <= 0) return;
  size_t bytes = static_cast<size_t>(read_res);
  buffer[bytes] = '\0';

  try {
    ParsedRequest req = parse_request(std::string(buffer, bytes));
    std::string url = std::string(req.host) + std::string(req.path);

    auto cached = cache.get(url);
    if (cached.has_value()) {
      std::cout << "[CACHE] HIT: " << req.method << " " << req.path << std::endl;
      std::string response = cached.value();
      inject_cache_header(response, "X-Cache: HIT\r\n");
      client.write_n(response.data(), response.size());
      return;
    }

    auto ticket = coalescer.start_or_join(url);
    if (ticket.is_owner) {
      auto double_check = cache.get(url);
      if (double_check.has_value()) {
        coalescer.complete(url, *ticket.signal);
        std::cout << "[CACHE] HIT: " << req.method << " " << req.path << std::endl;
        std::string response = double_check.value();
        inject_cache_header(response, "X-Cache: HIT\r\n");
        client.write_n(response.data(), response.size());
        return;
      }

      try {
        OriginInfo target = parse_origin(origin);
        std::string response = fetch_from_backend(target.host, target.port, req.path);
        cache.put(url, response);
        std::cout << "[CACHE] MISS: " << req.method << " " << req.path << std::endl;
        inject_cache_header(response, "X-Cache: MISS\r\n");
        coalescer.complete(url, *ticket.signal);
        client.write_n(response.data(), response.size());
      } catch (...) {
        coalescer.fail(url, *ticket.signal, std::current_exception());
        throw;
      }
    } else {
      try {
        ticket.done.get();
        auto new_cached = cache.get(url);
        if (!new_cached.has_value()) {
          throw ProxyException(500, "Internal Server Error", "Coalesced response not found in cache");
        }
        std::cout << "[CACHE] HIT (Coalesced): " << req.method << " " << req.path << std::endl;
        std::string response = new_cached.value();
        inject_cache_header(response, "X-Cache: HIT\r\n");
        client.write_n(response.data(), response.size());
        return;
      } catch (const ProxyException&) {
        throw;
      } catch (const std::exception& e) {
        throw ProxyException(502, "Bad Gateway", "Upstream request failed or owner thread crashed: " + std::string(e.what()));
      }
    }
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

void inject_cache_header(std::string& response, const std::string& header) {
  auto pos = response.find("\r\n\r\n");
  if (pos != std::string::npos) response.insert(pos + 2, header);
}

std::string fetch_from_backend(const std::string& host, int port, const std::string& path) {
  thread_local std::unique_ptr<httplib::Client> backend = nullptr;
  if (!backend) {  // Configuration
    backend = std::make_unique<httplib::Client>(host, port);
    backend->set_keep_alive(true);
    backend->set_connection_timeout(5, 0);
    backend->set_read_timeout(10, 0);
    backend->set_write_timeout(5, 0);
    backend->set_follow_location(true);
    backend->enable_server_certificate_verification(false);
  }

  // Fetch using httplib natively!
  auto res = backend->Get(path.c_str());
  if (!res) throw ProxyException(502, "Bad Gateway", "Backend unreachable");
  if (res->status != 200) throw ProxyException(res->status, "Upstream Error", "Failed");
  std::string full_response = "HTTP/1.1 200 OK\r\n";

  // Forward safe headers
  for (const auto& header : res->headers) {
    if (header.first == "Content-Length" || header.first == "Transfer-Encoding") continue;
    full_response += header.first + ": " + header.second + "\r\n";
  }

  // Add exact sizes and cache markers
  full_response += "Content-Length: " + std::to_string(res->body.size()) + "\r\n\r\n";
  full_response += res->body;

  return full_response;
}