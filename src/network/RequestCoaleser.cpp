#include "RequestCoaleser.hpp"

RequestCoalescer::Ticket RequestCoalescer::start_or_join(const std::string &key){
    std::lock_guard<std::mutex> lock(mtx);
    auto it = in_flight.find(key);
    if(it != in_flight.end()){
        return Ticket{false, it->second, std::nullopt};
    }

    std::promise<void> signal;
    std::shared_future<void> fut = signal.get_future().share();
    in_flight.emplace(key, fut);

    return Ticket{true, fut, std::move(signal)};
}

void RequestCoalescer::complete(const std::string& key, std::promise<void>& signal){
    {
        std::lock_guard<std::mutex> lock(mtx);
        in_flight.erase(key);
    }
    signal.set_value();
}

void RequestCoalescer::fail(const std::string& key, std::promise<void>& signal, std::exception_ptr ex){
    {
        std::lock_guard<std::mutex> lock(mtx);
        in_flight.erase(key);
    }
    signal.set_exception(ex);
}