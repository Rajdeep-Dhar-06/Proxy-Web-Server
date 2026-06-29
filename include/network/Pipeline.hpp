#pragma once
#include <memory>

#include "./Middleware.hpp"

class Pipeline {
  std::shared_ptr<Middleware> head;
  std::shared_ptr<Middleware> tail;

 public:
  void add_handler(std::shared_ptr<Middleware> handler) {
    if (!head) {
      head = handler;
      tail = handler;
      return;
    }
    tail->set_next(handler);
    tail = handler;
  }

  void execute(HttpContext& ctx) {
    if (head) {
      head->process(ctx);
    }
  }
};