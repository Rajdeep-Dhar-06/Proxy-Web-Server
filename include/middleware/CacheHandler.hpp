#pragma once
#include <memory>

#include "Middleware.hpp"
#include "cache/CacheStrategy.hpp"
#include "network/HttpContext.hpp"
#include "network/RequestCoalescer.hpp"

class CacheHandler : public Middleware {
 public:
  // Constructor
  CacheHandler(std::shared_ptr<ICache> cache_strategy, std::shared_ptr<RequestCoalescer> coalescer_ptr);

  // Core processing
  void process(HttpContext& ctx) override;

 private:
  // Helpers
  bool respond_from_cache(HttpContext& ctx, const std::string& key);
  void handle_as_owner(HttpContext& ctx, const std::string& key);
  void handle_as_waiter(HttpContext& ctx, RequestCoalescer::Ticket& ticket);

  // Members
  std::shared_ptr<ICache> cache;
  std::shared_ptr<RequestCoalescer> coalescer;
};
