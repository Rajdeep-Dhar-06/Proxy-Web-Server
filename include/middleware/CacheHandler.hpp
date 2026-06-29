#pragma once
#include "network/Middleware.hpp"
#include "cache/ICache.hpp"
#include "network/RequestCoalescer.hpp"
#include <memory>

class CacheHandler : public Middleware {
private:
    std::shared_ptr<ICache> cache;
    std::shared_ptr<RequestCoalescer> coalescer;

public:
    CacheHandler(std::shared_ptr<ICache> cache, std::shared_ptr<RequestCoalescer> coalescer);
    void process(HttpContext& ctx) override;
};
