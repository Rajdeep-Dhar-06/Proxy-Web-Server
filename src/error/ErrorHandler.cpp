#include "error/ErrorHandler.hpp"

#include "network/HttpContext.hpp"

ProxyException::ProxyException(int code, const std::string& err_msg) : status_code(code), error_message(err_msg) {}

const char* ProxyException::what() const noexcept { return error_message.c_str(); }

int ProxyException::get_status_code() const { return status_code; }

void ProxyException::populate_error_response(HttpResponse& response) const {
  response.status_code = status_code;
  response.headers.clear();
  response.headers["Connection"] = "close";
  response.headers["Content-Type"] = "text/html";
  response.body = "<html><body><h1>" + std::to_string(status_code) + " " + get_http_message() + "</h1></body></html>";
  response.ttl = 0;
}

std::string ProxyException::get_http_message() const {
  switch (status_code) {
    case 400:
      return "Bad Request";
    case 401:
      return "Unauthorized";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 411:
      return "Length Required";
    case 431:
      return "Request Header Fields Too Large";
    case 500:
      return "Internal Server Error";
    case 502:
      return "Bad Gateway";
    case 504:
      return "Gateway Timeout";
    default:
      return "Error";
  }
}
