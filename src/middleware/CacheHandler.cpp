// CacheHandler.cpp
#include "middleware/CacheHandler.hpp"

#include "config/config.hpp"
#include "error/ErrorHandler.hpp"
#include "logger/Logger.hpp"
#include "network/HttpContext.hpp"
#include "utils/http_utils.hpp"

static bool has_identity(const HttpContext& ctx) {
  return ctx.request.headers.count("authorization") > 0 || ctx.request.headers.count("cookie") > 0;
}

static bool is_explicitly_public(const HttpContext& ctx) {
  auto it = ctx.response.headers.find("cache-control");
  return it != ctx.response.headers.end() && it->second.find("public") != std::string::npos;
}

CacheHandler::CacheHandler(std::shared_ptr<ICache> cache_strategy, std::shared_ptr<RequestCoalescer> coalescer_ptr)
    : cache(std::move(cache_strategy)), coalescer(std::move(coalescer_ptr)) {}

void CacheHandler::process(HttpContext& ctx) {
  const std::string base_path = ctx.request.host + ctx.request.path;

  bool is_cacheable_method = (ctx.request.method == "GET" || ctx.request.method == "HEAD");

  if (!is_cacheable_method) {
    // Forward mutations (POST, PUT, DELETE, etc) directly to the backend
    Middleware::process(ctx);

    if (ctx.response.status_code >= 200 && ctx.response.status_code < 300) {
      cache->remove("GET:" + base_path);
      cache->remove("HEAD:" + base_path);
    }
    return;
  }

  const std::string cache_key = ctx.request.method + ":" + base_path;

  if (has_identity(ctx)) {
    Logger::get_instance().log("[SECURITY]\tIdentity detected. Bypassing cache for " + ctx.request.path, LoggerLevel::DEBUG);

    Middleware::process(ctx);  // Fetch directly from backend

    if (is_explicitly_public(ctx) && ctx.response.ttl > 0) { // cache if public and ttl greater than 0
      cache->put(cache_key, ctx.response, ctx.response.ttl);
    }

    ctx.response.headers["X-Cache"] = "BYPASS";
    return;
  }

  // Serve standard anonymous traffic
  if (respond_from_cache(ctx, cache_key)) return;

  // Coalesce concurrent cache misses
  auto ticket = coalescer->start_or_join(cache_key);
  if (ticket.is_owner) {
    handle_as_owner(ctx, cache_key);
  } else {
    handle_as_waiter(ctx, ticket);
  }
}

bool CacheHandler::respond_from_cache(HttpContext& ctx, const std::string& key) {
  auto cached = cache->get(key);
  if (!cached.has_value()) return false;

  Logger::get_instance().log("[CACHE HIT]\t" + ctx.request.method + " " + config.ORIGIN + ctx.request.path, LoggerLevel::INFO);
  ctx.response = cached.value();
  ctx.response.headers["X-Cache"] = "HIT";
  http::send_response(ctx);
  return true;
}

void CacheHandler::handle_as_owner(HttpContext& ctx, const std::string& key) {
  // Double-check the cache before fetching from backend. This guards against the race
  // condition where a concurrent request has finished fetching and populated the cache
  // in the window between our initial cache-miss check and joining the coalescer.
  if (respond_from_cache(ctx, key)) {
    return;
  }

  try {
    Middleware::process(ctx);  // next handler fetches from backend, fills ctx.response

    auto result = std::make_shared<HttpResponse>(ctx.response);
    if (ctx.response.ttl > 0) {
      cache->put(key, ctx.response, ctx.response.ttl);
    }

    Logger::get_instance().log("[CACHE MISS]\t" + ctx.request.method + " " + config.ORIGIN + ctx.request.path, LoggerLevel::INFO);
    ctx.response.headers["X-Cache"] = "MISS";
    coalescer->complete(key, result);
    http::send_response(ctx);
  } catch (...) {
    coalescer->fail(key, std::current_exception());
    throw;
  }
}

void CacheHandler::handle_as_waiter(HttpContext& ctx, RequestCoalescer::Ticket& ticket) {
  std::shared_ptr<HttpResponse> result;
  try {
    result = ticket.done.get();  // rethrows whatever fail() set, if anything
  } catch (const ProxyException&) {
    throw;
  } catch (const std::exception& e) {
    throw ProxyException(502, "Upstream request failed: " + std::string(e.what()));
  }

  Logger::get_instance().log("[COALESCED HIT]\t" + ctx.request.method + " " + config.ORIGIN + ctx.request.path, LoggerLevel::INFO);
  ctx.response = *result;
  ctx.response.headers["X-Cache"] = "HIT";
  http::send_response(ctx);
}
