#include "middleware/UpstreamHandler.hpp"

#include <memory>

#include "config/config.hpp"
#include "error/ErrorHandler.hpp"
#include "logger/Logger.hpp"
#include "utils/http_utils.hpp"
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

  // Adding headers
  httplib::Headers headers_to_send;
  for (const auto& [key, val] : ctx.request.headers) {
    if (key == "host" || key == "content-length") continue;
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
    Logger::get_instance().log("[BACKEND ERROR]\t" + ctx.request.method + " " + origin + ctx.request.path + " (Unreachable)",
                               LoggerLevel::ERROR);
    throw ProxyException(502, "Bad Gateway", "Backend unreachable");
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

  populate_response(ctx, *res);
}
