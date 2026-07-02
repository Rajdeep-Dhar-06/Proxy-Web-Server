#pragma once
#include "Middleware.hpp"

class ParseHandler : public Middleware {
 public:
  // Core processing
  void process(HttpContext& ctx) override;
};
