#include "middleware/ParseHandler.hpp"

#include <exception>

#include "error/ErrorHandler.hpp"
#include "logger/Logger.hpp"
#include "utils/http_utils.hpp"

void ParseHandler::process(HttpContext& ctx) {
  try {
    read_and_parse_request(ctx);
    Middleware::process(ctx);
    send_response(ctx);

  } catch (const SocketClosedException&) {
    return;  // Client connection closed; abort pipeline execution

  } catch (const ProxyException& e) {
    e.populate_error_response(ctx.response);
    send_response(ctx);

  } catch (const std::exception& e) {
    Logger::get_instance().log("Malformed HTTP request: " + std::string(e.what()), LoggerLevel::WARNING);
    ProxyException(400, "Bad Request", "Malformed HTTP request").populate_error_response(ctx.response);
    send_response(ctx);
  }
}