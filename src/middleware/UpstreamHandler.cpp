#include "middleware/UpstreamHandler.hpp"

#include "config/config.hpp"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <memory>

#include "error/ErrorHandler.hpp"
#include "logger/Logger.hpp"
#include "vendor/httplib.h"

UpstreamHandler::UpstreamHandler(std::string origin_server) : origin(origin_server) {}

void UpstreamHandler::process(HttpContext& ctx) {
  // thread-local client persistent connection to backend origin
  thread_local std::unique_ptr<httplib::Client> backend = nullptr;
  if (!backend) {
    backend = std::make_unique<httplib::Client>(origin);
    backend->set_keep_alive(true);
    backend->set_connection_timeout(config.CONNECT_TIMEOUT, 0);
    backend->set_read_timeout(config.READ_TIMEOUT, 0);
    backend->set_write_timeout(config.WRITE_TIMEOUT, 0);
    backend->set_follow_location(true);
    backend->enable_server_certificate_verification(false);
  }

  // Fetch using httplib
  Logger::get_instance().log("Fetching from upstream: " + ctx.request.path, LoggerLevel::INFO);
  auto res = backend->Get(ctx.request.path.c_str());

  if (!res) {
    Logger::get_instance().log("Backend unreachable to " + ctx.request.path, LoggerLevel::ERROR);
    throw ProxyException(502, "Bad Gateway", "Backend unreachable");
  }

  if (res->status != 200) {
    Logger::get_instance().log("Upstream returned " + std::to_string(res->status) + " to " + ctx.request.path, LoggerLevel::ERROR);
    throw ProxyException(res->status, "Upstream Error", "Failed");
  }

  size_t content_length = 0;
  if (res->has_header("Content-Length")) {
    content_length = std::stoull(res->get_header_value("Content-Length"));  // overshoot, wont be this much
  }

  bool exceeds_cacheable = false;
  if (content_length > config.MAX_CACHEABLE) exceeds_cacheable = true;

  // Populate HttpResponse inside ctx
  ctx.response.status_code = res->status;
  ctx.response.body = res->body;
  ctx.response.ttl = exceeds_cacheable ? 0 : config.TTL;

  if (res->has_header("Cache-Control")) {
    std::string cache_control = res->get_header_value("Cache-Control");

    if (cache_control.find("no-store") != std::string::npos || cache_control.find("no-cache") != std::string::npos ||
        cache_control.find("private") != std::string::npos) {
      //
    } else {
      size_t pos = cache_control.find("max-age=");
      if (pos != std::string::npos) {
        std::string num_str = cache_control.substr(pos + 8);
        size_t comma_pos = num_str.find(',');
        if (comma_pos != std::string::npos) {
          num_str = num_str.substr(0, comma_pos);
        }
        try {
          ctx.response.ttl = std::stoi(num_str);
        } catch (...) {
          ctx.response.ttl = config.TTL;
        }
      }
    }
  }

  // Forward safe headers from the origin response
  for (const auto& [key, value] : res->headers) {
    if (key == "Content-Length" || key == "Transfer-Encoding") continue;
    ctx.response.headers[key] = value;
  }
}
