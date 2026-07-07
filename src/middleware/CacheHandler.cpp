// CacheHandler.cpp
#include "middleware/CacheHandler.hpp"

#include "config/config.hpp"
#include "error/ErrorHandler.hpp"
#include "logger/Logger.hpp"
#include "network/HttpContext.hpp"
#include "utils/http_utils.hpp"

CacheHandler::CacheHandler(std::shared_ptr<ICache> cache, std::shared_ptr<RequestCoalescer> coalescer)
    : cache(std::move(cache)), coalescer(std::move(coalescer)) {}

void CacheHandler::process(HttpContext& ctx) {
  const std::string cache_key = ctx.request.host + ctx.request.path;

  if (ctx.request.method != "GET") {
    Middleware::process(ctx);  // calls the next middleware module

    if (ctx.response.status_code >= 200 && ctx.response.status_code < 300) {
      cache->remove(cache_key);
    }

    return;
  }

  if (respond_from_cache(ctx, cache_key, "HIT")) return;

  auto ticket = coalescer->start_or_join(cache_key);
  if (ticket.is_owner) {
    handle_as_owner(ctx, cache_key);
  } else {
    handle_as_waiter(ctx, cache_key, ticket);
  }
}

bool CacheHandler::respond_from_cache(HttpContext& ctx, const std::string& key, const char* hit_label) {
  auto cached = cache->get(key);
  if (!cached.has_value()) return false;

  Logger::get_instance().log("[CACHE HIT]\t" + ctx.request.method + " " + config.ORIGIN + ctx.request.path, LoggerLevel::INFO);
  ctx.response = cached.value();
  ctx.response.headers["X-Cache"] = "HIT";
  http::send_response(ctx);
  return true;
}

void CacheHandler::handle_as_owner(HttpContext& ctx, const std::string& key) {
  if (respond_from_cache(ctx, key, "HIT")) {
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
    http::send_response(ctx);
    coalescer->complete(key, result);
  } catch (...) {
    coalescer->fail(key, std::current_exception());
    throw;
  }
}

void CacheHandler::handle_as_waiter(HttpContext& ctx, const std::string& key, RequestCoalescer::Ticket& ticket) {
  std::shared_ptr<HttpResponse> result;
  try {
    result = ticket.done.get();  // rethrows whatever fail() set, if anything
  } catch (const ProxyException&) {
    throw;
  } catch (const std::exception& e) {
    throw ProxyException(502, "Bad Gateway", "Upstream request failed: " + std::string(e.what()));
  }

  Logger::get_instance().log("[COALESCED HIT]\t" + ctx.request.method + " " + config.ORIGIN + ctx.request.path, LoggerLevel::INFO);
  ctx.response = *result;
  ctx.response.headers["X-Cache"] = "HIT";
  http::send_response(ctx);
}
