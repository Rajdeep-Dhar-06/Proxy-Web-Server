#include "utils/inject_cache_header.hpp"

void inject_cache_header(HttpContext& ctx, const std::string& cache_status) {
    ctx.response.headers["X-Cache"] = cache_status;
}
