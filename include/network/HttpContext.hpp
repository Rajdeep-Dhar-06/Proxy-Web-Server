#pragma once
#include <sockpp/tcp_socket.h>

#include <string>
#include <unordered_map>
#include <vector>

struct HttpRequest {
  std::string method;
  std::string path;
  std::string host;
  // std::unordered_map<std::string, std::string> headers;
  // std::string body;
};

struct HttpResponse {
  int status_code;
  std::unordered_map<std::string, std::string> headers;
  std::string body;
  bool committed = false;
  int ttl = 60;
};

class HttpContext {
 public:
  HttpRequest request;
  HttpResponse response;
  sockpp::tcp_socket& socket;  // Reference to the active TCP connection

  // Shared state flags for middleware coordination
  bool skip_cache = false;

  HttpContext(sockpp::tcp_socket& sock) : socket(sock) {}
};