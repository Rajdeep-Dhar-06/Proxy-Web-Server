#pragma once
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "network/HttpContext.hpp"

class RequestCoalescer {
 public:
  // Types
  struct Ticket {
    bool is_owner;
    std::shared_future<std::shared_ptr<HttpResponse>> done;  // owner's result
  };

  // Coalescing management
  Ticket start_or_join(const std::string& key);
  void complete(const std::string& key, std::shared_ptr<HttpResponse> result);
  void fail(const std::string& key, std::exception_ptr ex);

 private:
  // Types
  struct InFlightRequest {
    std::shared_future<std::shared_ptr<HttpResponse>> future;
    std::promise<std::shared_ptr<HttpResponse>> promise;
  };

  // Members
  std::mutex mtx;
  std::unordered_map<std::string, InFlightRequest> in_flight;
};