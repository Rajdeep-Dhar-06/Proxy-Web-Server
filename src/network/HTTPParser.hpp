#pragma once
#include <string>

struct ParsedRequest {
  std::string method; // "GET"
  std::string path;   // "/products"
  std::string host;   // "dummyjson.com"
};

ParsedRequest parse_request(const std::string &http_text);
