#pragma once
#include <exception>
#include <string>

#include "network/HttpContext.hpp"

class SocketClosedException : public std::exception {
 public:
  // Error details
  const char* what() const noexcept override { return "Socket closed"; }
};

// HTTP utility functions

// Maps HTTP status codes to standard reason phrases
std::string get_status_message(int status_code);

// Reads raw HTTP request bytes from client socket, parses it using picohttpparser
void read_and_parse_request(HttpContext& ctx);

// Serializes an HttpResponse to a raw HTTP/1.1 response string
std::string serialize_response(HttpResponse& response);

// Sends the response in HttpContext back to the client socket and marks it committed
void send_response(HttpContext& ctx);