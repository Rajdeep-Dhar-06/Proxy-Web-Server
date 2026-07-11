#include "middleware/ParseHandler.hpp"

#include <exception>

#include "config/config.hpp"
#include "error/ErrorHandler.hpp"
#include "logger/Logger.hpp"
#include "utils/http_utils.hpp"

void ParseHandler::process(HttpContext& ctx) {
  int requests_served = 0;

  while (true) {
    ctx.request = HttpRequest{};
    ctx.response = HttpResponse{};

    try {
      http::read_and_parse_request(ctx);
    } catch (const SocketClosedException&) {
      return;  // client closed, or the idle read timeout fired -- same handling either way
    } catch (const ProxyException& e) {
      ctx.keep_alive = false;
      e.populate_error_response(ctx.response);
      http::send_response(ctx);
      return;
    } catch (const std::exception& e) {
      Logger::get_instance().log("Malformed HTTP request: " + std::string(e.what()), LoggerLevel::WARNING);
      ctx.keep_alive = false;
      ProxyException(400, "Bad Request", "Malformed HTTP request").populate_error_response(ctx.response);
      http::send_response(ctx);
      return;
    }

    ctx.keep_alive = http::wants_keep_alive(ctx.request) && (++requests_served < config.MAX_REQUESTS_PER_CONNECTION);

    try {
      Middleware::process(ctx);
      http::send_response(ctx);  // no-op if a downstream handler already sent (committed guard handles it)
    } catch (const ProxyException& e) {
      ctx.keep_alive = false;
      e.populate_error_response(ctx.response);
      http::send_response(ctx);
    } catch (const std::exception& e) {
      Logger::get_instance().log("Unhandled exception: " + std::string(e.what()), LoggerLevel::ERROR);
      ctx.keep_alive = false;
      ProxyException(500, "Internal Server Error", e.what()).populate_error_response(ctx.response);
      http::send_response(ctx);
    }

    if (!ctx.keep_alive) return;
  }
}