#pragma once
#include <memory>

#include "../middleware/Middleware.hpp"

class Pipeline {
 public:
  // Pipeline construction
  void add_handler(std::shared_ptr<Middleware> handler) {
    if (!head) {
      head = handler;
      tail = std::move(handler);
      return;
    }
    tail->set_next(handler);
    tail = std::move(handler);
  }

  // Execution
  void execute(HttpContext& ctx) {
    if (head) {
      head->process(ctx);
    }
  }

 private:
  // Members
  std::shared_ptr<Middleware> head;
  std::shared_ptr<Middleware> tail;
};