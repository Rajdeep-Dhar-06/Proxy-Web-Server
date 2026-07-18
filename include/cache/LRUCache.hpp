#pragma once
#include <chrono>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "./CacheStrategy.hpp"

using timepoint = std::chrono::steady_clock::time_point;

struct CacheNode {
  // Members
  timepoint expiration;
  std::string key;
  HttpResponse response;

  // Constructor
  CacheNode(std::string node_key, HttpResponse val, timepoint expires_at)
      : expiration(expires_at), key(std::move(node_key)), response(std::move(val)) {}
};

class LRUCache : public ICache {
 public:
  // Constructor and destructor
  LRUCache(size_t cache_capacity);
  ~LRUCache() override = default;

  // Cache operations
  std::optional<HttpResponse> get(const std::string& key) override;
  void put(const std::string& key, HttpResponse response, int ttl) override;
  void remove(const std::string& key) override;

 private:
  // Members
  size_t capacity;
  std::list<CacheNode> cacheList;
  std::unordered_map<std::string, std::list<CacheNode>::iterator> cacheMap; 
  std::mutex cacheMutex;                                                     
};