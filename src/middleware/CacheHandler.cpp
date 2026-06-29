#include "middleware/CacheHandler.hpp"
#include "error/ErrorHandler.hpp"
#include <iostream>
#include <exception>

// Manual forward declarations of utility functions
void inject_cache_header(std::string& response, const std::string& header);
std::string serialize_response(HttpContext& ctx);
void send_response(HttpContext& ctx);

CacheHandler::CacheHandler(std::shared_ptr<ICache> cache, std::shared_ptr<RequestCoalescer> coalescer) 
    : cache(cache), coalescer(coalescer) {}

void CacheHandler::process(HttpContext& ctx) {
  std::string url = ctx.request.host + ctx.request.path;

  // Primary cache lookup check.
  auto cached = cache->get(url);
  if (cached.has_value()) {
    std::cout << "[CACHE] HIT: " << ctx.request.method << " " << ctx.request.path << std::endl;
    std::string response = cached.value();
    inject_cache_header(response, "X-Cache: HIT\r\n");
    ctx.socket.write_n(response.data(), response.size());
    return;
  }

  // Attempt to coalesce duplicate concurrent requests targeting the same URL.
  auto ticket = coalescer->start_or_join(url);
  if (ticket.is_owner) {
    // Double check in case another request populated the cache right before we claimed ownership.
    auto cached = cache->get(url);
    if (cached.has_value()) {
      coalescer->complete(url);
      std::cout << "[CACHE] HIT: " << ctx.request.method << " " << ctx.request.path << std::endl;
      std::string response = cached.value();
      inject_cache_header(response, "X-Cache: HIT\r\n");
      ctx.socket.write_n(response.data(), response.size());
      return;
    }

    try {
      // Call the next middleware in the pipeline (which will fetch from backend and populate ctx.response)
      Middleware::process(ctx);
      
      // Serialize the response that was populated by upstream/next middleware to store it in cache
      std::string response = serialize_response(ctx);
      
      // Cache the newly retrieved content if caching is enabled (ttl > 0)
      if (ctx.response.ttl > 0) {
        cache->put(url, response, ctx.response.ttl);
      }
      
      std::cout << "[CACHE] MISS: " << ctx.request.method << " " << ctx.request.path << std::endl;
      
      // We need to inject the X-Cache MISS header before sending.
      ctx.response.headers["X-Cache"] = "MISS";

      // Send the response back to the client
      send_response(ctx);
      
      // Unblock waiting sibling threads.
      coalescer->complete(url);
    } catch (...) {
      // Broadcast the failure to waiting threads to prevent deadlock, then rethrow.
      coalescer->fail(url, std::current_exception());
      throw;
    }
  } else {
    // Waiter thread: block until the owner thread resolves the cache promise.
    try {
      ticket.done.get();
      auto cached = cache->get(url);
      if (!cached.has_value()) {
        throw ProxyException(500, "Internal Server Error", "Coalesced response not found in cache");
      }
      std::cout << "[CACHE] HIT (Coalesced): " << ctx.request.method << " " << ctx.request.path << std::endl;
      std::string response = cached.value();
      inject_cache_header(response, "X-Cache: HIT\r\n");
      ctx.socket.write_n(response.data(), response.size());
      return;
    } catch (const ProxyException&) {
      throw;
    } catch (const std::exception& e) {
      throw ProxyException(502, "Bad Gateway", "Upstream request failed or owner thread crashed: " + std::string(e.what()));
    }
  }
}