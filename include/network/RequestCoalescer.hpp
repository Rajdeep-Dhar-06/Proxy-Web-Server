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
    bool is_owner;                                           ///< True if this thread must perform the request
    std::shared_future<std::shared_ptr<HttpResponse>> done;  ///< Resolves to the owner's actual result
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
  std::mutex mtx;                                              ///< Protects the in_flight map
  std::unordered_map<std::string, InFlightRequest> in_flight;  ///< Maps URL to the active in-flight request
};