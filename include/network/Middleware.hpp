#pragma once
#include <memory>
#include "HttpContext.hpp"

class Middleware {
protected:
    std::shared_ptr<Middleware> next_;
public:
    virtual ~Middleware() = default;

    void set_next(std::shared_ptr<Middleware> next) {
        next_ = next;
    }

    virtual void process(HttpContext& ctx) {
        if (next_) {
            next_->process(ctx);
        }
    }
};