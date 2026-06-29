#pragma once
#include "network/HttpContext.hpp"

#include <exception>

class SocketClosedException : public std::exception {
public:
  const char* what() const noexcept override {
    return "Socket closed";
  }
};

// Reads raw HTTP request bytes from client socket, parses it using picohttpparser, 
// and populates the HttpContext request object.
void read_and_parse_request(HttpContext& ctx);
