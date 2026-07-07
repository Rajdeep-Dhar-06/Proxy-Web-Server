#pragma once
#include <sockpp/tcp_socket.h>

#include <string>
#include <unordered_map>

struct HttpRequest {
  // Members
  std::string method;
  std::string path;
  std::string host;
  std::unordered_map<std::string, std::string> headers;
  std::string body;
};

struct HttpResponse {
  // Members
  std::unordered_map<std::string, std::string> headers;
  std::string body;
  int status_code;
  int ttl;
  bool committed = false;
};

class HttpContext {
 public:
  // Constructor
  HttpContext(sockpp::tcp_socket& sock) : socket(sock) {}

  // Members
  HttpRequest request;
  HttpResponse response;
  sockpp::tcp_socket& socket;

  // Shared state flags for middleware coordination
  bool skip_cache = false;
};