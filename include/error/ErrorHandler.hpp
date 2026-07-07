#pragma once
#include <exception>
#include <string>

struct HttpResponse;

class ProxyException : public std::exception {
 public:
  // Constructor
  explicit ProxyException(int status_code, const std::string& http_message, const std::string& error_message);

  // Error details
  const char* what() const noexcept override;
  int get_status_code() const;
  const std::string& get_http_message() const;

  // HTTP response handling
  void populate_error_response(HttpResponse& response) const;

 private:
  // Members
  int status_code;
  std::string http_message;
  std::string error_message;
};