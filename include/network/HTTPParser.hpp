#pragma once
#include <sockpp/tcp_socket.h>
#include <string>
#include <exception>

class SocketClosedException : public std::exception {
public:
  const char* what() const noexcept override {
    return "Socket closed";
  }
};

struct ParsedRequest {
  std::string method; ///< The HTTP method 
  std::string path;   ///< The relative request path
  std::string host;   ///< The target host domain name
};

ParsedRequest parse_request(sockpp::tcp_socket& client);
