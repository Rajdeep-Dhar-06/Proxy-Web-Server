#include "error/ErrorHandler.hpp"

ProxyException::ProxyException(int status_code, const std::string& http_message, const std::string& error_message)
    : status_code(status_code), http_message(http_message), error_message(error_message) {}

const char* ProxyException::what() const noexcept { 
  return error_message.c_str(); 
}

int ProxyException::get_status_code() const { 
  return status_code; 
}

const std::string& ProxyException::get_http_message() const { 
  return http_message; 
}

std::string ProxyException::generate_http_response(int code, const std::string& message) {
  // Construct the raw HTTP response headers
  std::string response = "HTTP/1.1 " + std::to_string(code) + " " + message + "\r\n";
  response += "Connection: close\r\n";
  response += "Content-Type: text/html\r\n";

  // Build a simple HTML error page body to send to the client
  std::string body = "<html><body><h1>" + std::to_string(code) + " " + message + "</h1></body></html>";
  
  // Append content length header and the response body
  response += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
  response += body;
  
  return response;
}
