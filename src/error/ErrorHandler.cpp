#include "error/ErrorHandler.hpp"

#include "network/HttpContext.hpp"

ProxyException::ProxyException(int status_code, const std::string& http_message, const std::string& error_message)
    : status_code(status_code), http_message(http_message), error_message(error_message) {}

const char* ProxyException::what() const noexcept { return error_message.c_str(); }

int ProxyException::get_status_code() const { return status_code; }

const std::string& ProxyException::get_http_message() const { return http_message; }

void ProxyException::populate_error_response(HttpResponse& response) const {
  response.status_code = status_code;
  response.headers.clear();
  response.headers["Connection"] = "close";
  response.headers["Content-Type"] = "text/html";
  response.body = "<html><body><h1>" + std::to_string(status_code) + " " + http_message + "</h1></body></html>";
  response.ttl = 0;
}
