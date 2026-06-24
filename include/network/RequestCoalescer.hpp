/**
 * @file RequestCoalescer.hpp
 * @brief Thread-safe request coalescing mechanism to prevent cache stampedes.
 *
 * This class coordinates concurrent requests for the same URI, ensuring that only one
 * thread (the owner) queries the upstream origin server while all subsequent concurrent
 * threads (the waiters) block on the owner's result.
 */

#pragma once
#include <exception>
#include <future>
#include <mutex>
#include <string>
#include <unordered_map>

/**
 * @class RequestCoalescer
 * @brief Prevents duplicate concurrent requests to the backend for identical resources.
 *
 * RequestCoalescer acts as a gatekeeper. By utilizing futures and promises, it aggregates
 * multiple matching concurrent requests into a single upstream request, shielding the origin
 * backend from thundering herds.
 */
class RequestCoalescer {
 private:
  struct InFlightRequest {
    std::shared_future<void> future;
    std::promise<void> promise;
  };

  std::mutex mtx;                                                       ///< Protects the in_flight map
  std::unordered_map<std::string, InFlightRequest> in_flight;           ///< Maps URL to the active in-flight request

 public:
  /**
   * @struct Ticket
   * @brief Token returned to threads entering the coalescer.
   *
   * Distinguishes whether the caller thread is responsible for executing the backend request (owner)
   * or should wait for another thread to complete the request (waiter).
   */
  struct Ticket {
    bool is_owner;                            ///< True if this thread must perform the request; False if it should wait
    std::shared_future<void> done;            ///< Future that resolves when the request finishes
  };

  /**
   * @brief Registers interest in a URL, checking if a concurrent request is already in progress.
   *
   * If a request is already in flight for the given key, returns a ticket to wait.
   * If no request is in flight, registers this request and returns a ticket declaring ownership.
   *
   * @param key The unique key (usually the request URL) representing the resource.
   * @return Ticket Token indicating ownership or wait future.
   */
  Ticket start_or_join(const std::string& key);

  /**
   * @brief Marks a coalesced request as successfully completed.
   *
   * Removes the key from the active registry and resolves the completion signal to unblock waiters.
   *
   * @param key The unique key representing the resource.
   */
  void complete(const std::string& key);

  /**
   * @brief Marks a coalesced request as failed.
   *
   * Removes the key from the active registry and propagates the exception to all waiting threads.
   *
   * @param key The unique key representing the resource.
   * @param ex Pointer to the exception that caused the failure.
   */
  void fail(const std::string& key, std::exception_ptr ex);
};
