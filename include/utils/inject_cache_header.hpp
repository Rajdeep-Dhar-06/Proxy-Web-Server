#pragma once
#include "network/HttpContext.hpp"
#include <string>

// Injects/updates the X-Cache header key in the HttpContext response headers map.
void inject_cache_header(HttpContext& ctx, const std::string& cache_status);
