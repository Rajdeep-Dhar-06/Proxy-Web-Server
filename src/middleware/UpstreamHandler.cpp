#include "middleware/UpstreamHandler.hpp"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "vendor/httplib.h"
#include "error/ErrorHandler.hpp"
#include <memory>

UpstreamHandler::UpstreamHandler(std::string origin_server) : origin(origin_server) {}

void UpstreamHandler::process(HttpContext& ctx) {
  // thread-local client persistent connection to backend origin
  thread_local std::unique_ptr<httplib::Client> backend = nullptr;
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
  auto res = backend->Get(ctx.request.path.c_str());
  if (!res) throw ProxyException(502, "Bad Gateway", "Backend unreachable");
  if (res->status != 200) throw ProxyException(res->status, "Upstream Error", "Failed");

  // Populate HttpResponse inside ctx
  ctx.response.status_code = res->status;
  ctx.response.body = res->body;

  ctx.response.ttl = 60; // default value
  if (res->has_header("Cache-Control")) {
    std::string cache_control = res->get_header_value("Cache-Control");

    if (cache_control.find("no-store") != std::string::npos ||
        cache_control.find("no-cache") != std::string::npos ||
        cache_control.find("private") != std::string::npos) {
      ctx.response.ttl = 60; // Temporary caching of 60s
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
          ctx.response.ttl = 60;  
        }
      }
    }
  }

  // Forward safe headers from the origin response
  for (const auto& header : res->headers) {
    if (header.first == "Content-Length" || header.first == "Transfer-Encoding") continue;
    ctx.response.headers[header.first] = header.second;
  }
}
