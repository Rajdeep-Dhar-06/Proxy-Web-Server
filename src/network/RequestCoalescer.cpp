#include "RequestCoalescer.hpp"

RequestCoalescer::Ticket RequestCoalescer::start_or_join(const std::string &key) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Check if there is already an active/pending backend request for this URL
    auto it = in_flight.find(key);
    if (it != in_flight.end()) {
        // Waiter case: return the shared future without a promise signal
        return Ticket{false, it->second, std::nullopt};
    }

    // Owner case: initialize a promise-future pair to manage state propagation
    std::promise<void> signal;
    std::shared_future<void> fut = signal.get_future().share();
    in_flight.emplace(key, fut);

    return Ticket{true, fut, std::move(signal)};
}

void RequestCoalescer::complete(const std::string& key, std::promise<void>& signal) {
    {
        // Scope the lock so we release it before triggering the promise (avoids lock contention)
        std::lock_guard<std::mutex> lock(mtx);
        in_flight.erase(key);
    }
    // Notify all waiting threads that the cached resource is ready
    signal.set_value();
}

void RequestCoalescer::fail(const std::string& key, std::promise<void>& signal, std::exception_ptr ex) {
    {
        // Scope the lock to release it before propagating the failure
        std::lock_guard<std::mutex> lock(mtx);
        in_flight.erase(key);
    }
    // Propagate the backend error/exception to all blocked threads
    signal.set_exception(ex);
}
