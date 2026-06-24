#include "network/RequestCoalescer.hpp"

RequestCoalescer::Ticket RequestCoalescer::start_or_join(const std::string& key) {
  std::lock_guard<std::mutex> lock(mtx);

  // Check if there is already an active/pending backend request for this URL
  auto it = in_flight.find(key);
  if (it != in_flight.end()) {
    // Waiter case: return the shared future
    return Ticket{false, it->second.future};
  }

  // Owner case: initialize a promise-future pair to manage state propagation
  std::promise<void> promise;
  std::shared_future<void> future = promise.get_future().share();
  in_flight.emplace(key, InFlightRequest{future, std::move(promise)});

  return Ticket{true, future};
}

void RequestCoalescer::complete(const std::string& key) {
  std::promise<void> promise;
  {
    // Scope the lock so we release it before triggering the promise (avoids lock contention)
    std::lock_guard<std::mutex> lock(mtx);
    auto it = in_flight.find(key);
    if (it != in_flight.end()) {
      promise = std::move(it->second.promise);
      in_flight.erase(it);
      // Notify all waiting threads that the cached resource is ready
      promise.set_value();
    }
  }
}

void RequestCoalescer::fail(const std::string& key, std::exception_ptr ex) {
  std::promise<void> promise;
  {
    // Scope the lock to release it before propagating the failure
    std::lock_guard<std::mutex> lock(mtx);
    auto it = in_flight.find(key);
    if (it != in_flight.end()) {
      promise = std::move(it->second.promise);
      in_flight.erase(it);
      // Propagate the backend error/exception to all blocked threads
      promise.set_exception(ex);
    }
  }
}
