#pragma once
#include "network/Middleware.hpp"

class ParseHandler : public Middleware {
public:
    void process(HttpContext& ctx) override;
};
