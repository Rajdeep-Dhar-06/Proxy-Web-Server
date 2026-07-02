#pragma once
#include <optional>
#include <string>

#include "network/HttpContext.hpp"

class ICache {
 public:
  // Constructor and destructor
  virtual ~ICache() = default;

  // Cache operations
  virtual std::optional<HttpResponse> get(const std::string& key) = 0;
  virtual void put(const std::string& key, HttpResponse value, int ttl) = 0;
};