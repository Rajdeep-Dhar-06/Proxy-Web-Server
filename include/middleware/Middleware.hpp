#pragma once
#include <memory>

#include "network/HttpContext.hpp"

class Middleware {
 public:
  // Constructor and destructor
  virtual ~Middleware() = default;

  // Chain management
  void set_next(std::shared_ptr<Middleware> nextMiddleware) { next = std::move(nextMiddleware); }

  // Core processing
  virtual void process(HttpContext& ctx) {
    if (next) {
      next->process(ctx);
    }
  }

 protected:
  // Members
  std::shared_ptr<Middleware> next;
};