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

#include "HttpContext.hpp"

HttpRequest parse_request(sockpp::tcp_socket& client);
