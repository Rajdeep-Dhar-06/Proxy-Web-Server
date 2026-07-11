#include "middleware/UpstreamHandler.hpp"

#include <memory>

#include "config/config.hpp"
#include "error/ErrorHandler.hpp"
#include "logger/Logger.hpp"
#include "utils/http_utils.hpp"
#include "vendor/httplib.h"

UpstreamHandler::UpstreamHandler(std::string origin_server) : origin(origin_server) {}

void UpstreamHandler::process(HttpContext& ctx) {
  auto init_backend = [this]() {
    auto client = std::make_unique<httplib::Client>(origin);
    client->set_keep_alive(true);
    client->set_connection_timeout(config.CONNECT_TIMEOUT, 0);
    client->set_read_timeout(config.READ_TIMEOUT, 0);
    client->set_write_timeout(config.WRITE_TIMEOUT, 0);
    client->set_follow_location(true);
    client->enable_server_certificate_verification(false);
    return client;
  };

  // thread-local client persistent connection to backend origin
  thread_local std::unique_ptr<httplib::Client> backend = nullptr;
  if (!backend) {
    backend = init_backend();
  }

  // Adding headers
  httplib::Headers headers_to_send;
  for (const auto& [key, val] : ctx.request.headers) {
    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);

    // RFC 7230 Hop-by-hop headers that MUST NOT be forwarded
    if (k == "host" || k == "content-length" || k == "connection" || k == "keep-alive" || k == "proxy-authenticate" ||
        k == "proxy-authorization" || k == "proxy-connection" || k == "te" || k == "trailer" || k == "transfer-encoding" ||
        k == "upgrade") {
      continue;
    }
    headers_to_send.insert({key, val});
  }

  // Fetch using httplib
  httplib::Request req;
  req.method = ctx.request.method;  // http method
  req.path = ctx.request.path;      // "/login"
  req.headers = headers_to_send;    // headers
  req.body = ctx.request.body;      // body

  Logger::get_instance().log("[SERVER FETCH]\t" + ctx.request.method + " " + origin + ctx.request.path, LoggerLevel::INFO);
  auto res = backend->send(req);

  if (!res) {
    bool is_idempotent =
        (ctx.request.method == "GET" || ctx.request.method == "HEAD" || ctx.request.method == "PUT" || ctx.request.method == "DELETE");

    bool failed_before_transmit = (res.error() == httplib::Error::Connection || res.error() == httplib::Error::ConnectionTimeout ||
                                   res.error() == httplib::Error::Write);

    if (is_idempotent || failed_before_transmit) {
      Logger::get_instance().log("[STALE SOCKET]\tReconnecting to " + origin, LoggerLevel::DEBUG);
      backend = nullptr;
      backend = init_backend();
      res = backend->send(req);
    }
  }

  if (!res) {
    std::string err_msg = httplib::to_string(res.error());
    Logger::get_instance().log("[BACKEND ERROR]\t" + ctx.request.method + " " + origin + ctx.request.path + " (" + err_msg + ")",
                               LoggerLevel::ERROR);

    throw ProxyException(502, "Bad Gateway", err_msg);
  }

  if (res->status >= 100 && res->status < 200) {
    Logger::get_instance().log(
        "[SERVER INFO]\t" + ctx.request.method + " " + origin + ctx.request.path + " (Status: " + std::to_string(res->status) + ")",
        LoggerLevel::INFO);
  } else if (res->status >= 200 && res->status < 300) {
    Logger::get_instance().log(
        "[SERVER SUCCESS]\t" + ctx.request.method + " " + origin + ctx.request.path + " (Status: " + std::to_string(res->status) + ")",
        LoggerLevel::INFO);
  } else if (res->status >= 300 && res->status < 400) {
    Logger::get_instance().log(
        "[SERVER REDIRECT]\t" + ctx.request.method + " " + origin + ctx.request.path + " (Status: " + std::to_string(res->status) + ")",
        LoggerLevel::INFO);
  } else if (res->status >= 400 && res->status < 500) {
    Logger::get_instance().log(
        "[CLIENT ERROR]\t" + ctx.request.method + " " + origin + ctx.request.path + " (Status: " + std::to_string(res->status) + ")",
        LoggerLevel::WARNING);
  } else if (res->status >= 500 && res->status < 600) {
    Logger::get_instance().log(
        "[SERVER ERROR]\t" + ctx.request.method + " " + origin + ctx.request.path + " (Status: " + std::to_string(res->status) + ")",
        LoggerLevel::ERROR);
  }

  http::populate_response(ctx, *res);
}
