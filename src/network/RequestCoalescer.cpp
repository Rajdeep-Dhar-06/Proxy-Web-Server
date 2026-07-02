#include "network/RequestCoalescer.hpp"

#include "logger/Logger.hpp"

RequestCoalescer::Ticket RequestCoalescer::start_or_join(const std::string& key) {
  std::lock_guard<std::mutex> lock(mtx);

  auto it = in_flight.find(key);
  if (it != in_flight.end()) {
    Logger::get_instance().log("Joining in-flight request for key: " + key, LoggerLevel::DEBUG);
    return Ticket{false, it->second.future};
  }

  std::promise<std::shared_ptr<HttpResponse>> promise;
  std::shared_future<std::shared_ptr<HttpResponse>> future = promise.get_future().share();
  in_flight.emplace(key, InFlightRequest{future, std::move(promise)});
  return Ticket{true, future};
}

void RequestCoalescer::complete(const std::string& key, std::shared_ptr<HttpResponse> result) {
  std::promise<std::shared_ptr<HttpResponse>> promise;
  {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = in_flight.find(key);
    if (it == in_flight.end()) return;
    promise = std::move(it->second.promise);
    in_flight.erase(it);
  }
  promise.set_value(std::move(result));
}

void RequestCoalescer::fail(const std::string& key, std::exception_ptr ex) {
  std::promise<std::shared_ptr<HttpResponse>> promise;
  {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = in_flight.find(key);
    if (it == in_flight.end()) return;
    promise = std::move(it->second.promise);
    in_flight.erase(it);
  }
  promise.set_exception(ex);
}
