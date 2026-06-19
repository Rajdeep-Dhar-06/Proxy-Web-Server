#include <exception>
#include <string>

class ProxyException : public std::exception {
 private:
  int status_code;
  std::string http_message;
  std::string error_message;

 public:
  explicit ProxyException(int status_code, const std::string& http_message, const std::string& error_message)
      : status_code(status_code), http_message(http_message), error_message(error_message) {}
  const char* what() const noexcept override { return error_message.c_str(); }
  int get_status_code() const { return status_code; }
  const std::string& get_http_message() const { return http_message; }
  static std::string generate_http_response(int code, const std::string& message) {
    std::string response = "HTTP/1.1 " + std::to_string(code) + " " + message + "\r\n";
    response += "Connection: close\r\n";
    response += "Content-Type: text/html\r\n";
    std::string body = "<html><body><h1>" + std::to_string(code) + " " + message + "</h1></body></html>";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
    response += body;
    return response;
  }
};