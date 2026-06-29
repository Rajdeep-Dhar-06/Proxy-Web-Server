#pragma once
#include "network/Middleware.hpp"
#include <string>

class UpstreamHandler : public Middleware {
private:
    std::string origin;

public:
    explicit UpstreamHandler(std::string origin_server);
    void process(HttpContext& ctx) override;
};
