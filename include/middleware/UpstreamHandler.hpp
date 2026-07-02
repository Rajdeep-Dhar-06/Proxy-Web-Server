#pragma once
#include <string>

#include "Middleware.hpp"

class UpstreamHandler : public Middleware {
 public:
  // Constructor
  explicit UpstreamHandler(std::string origin_server);

  // Core processing
  void process(HttpContext& ctx) override;

 private:
  // Members
  std::string origin;
};
