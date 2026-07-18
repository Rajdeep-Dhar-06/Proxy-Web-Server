#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "network/IConnection.hpp"

struct HttpRequest {
  // Members
  std::string method;
  std::string path;
  std::string host;
  std::unordered_map<std::string, std::string> headers;
  std::string body;
  int http_minor = 1;
};

struct HttpResponse {
  // Members
  std::unordered_map<std::string, std::string> headers;
  std::string body;
  int status_code = 0;
  int ttl = 0;
  bool committed = false;
};

class HttpContext {
 public:
  // Constructor
  explicit HttpContext(std::unique_ptr<IConnection> connection) : connection_(std::move(connection)) {}

  // Members
  HttpRequest request;
  HttpResponse response;

  IConnection& connection() { return *connection_; }

  // Shared state flags for middleware coordination
  bool keep_alive = false;

 private:
  std::unique_ptr<IConnection> connection_;
};