#pragma once
#include "network/HttpContext.hpp"
#include <string>

// Maps HTTP status codes to standard reason phrases.
std::string get_status_message(int status_code);

// Serializes the response inside HttpContext to a raw HTTP/1.1 response string.
std::string serialize_response(HttpContext& ctx);

// Sends the response in HttpContext back to the client socket and marks it committed.
void send_response(HttpContext& ctx);

// Parses a raw HTTP/1.1 response string into an HttpResponse structure.
// Useful for cache hit response recovery.
void parse_response_string(const std::string& raw_resp, HttpResponse& resp);
