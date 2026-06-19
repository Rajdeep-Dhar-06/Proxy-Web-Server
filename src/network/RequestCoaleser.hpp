#pragma once
#include <exception>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class RequestCoalescer {
private:
  std::mutex mtx;
  std::unordered_map<std::string, std::shared_future<void>> in_flight;

public:
  struct Ticket {
    bool is_owner;
    std::shared_future<void> done;
    std::optional<std::promise<void>> signal;
  };
  Ticket start_or_join(const std::string &key);
  void complete(const std::string &key, std::promise<void> &signal);
  void fail(const std::string &key, std::promise<void> &signal,
            std::exception_ptr ex);
};