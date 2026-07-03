#pragma once
#include <chrono>
#include <memory>
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
  std::weak_ptr<CacheNode> prev;
  std::shared_ptr<CacheNode> next;
  std::string key;
  HttpResponse response;

  // Constructor
  CacheNode(const std::string& key, HttpResponse val, timepoint expires_at)
      : key(key), response(std::move(val)), prev(), next(nullptr), expiration(expires_at) {}
};

class LRUCache : public ICache {
 public:
  // Constructor and destructor
  LRUCache(size_t cap);
  ~LRUCache();

  // Cache operations
  std::optional<HttpResponse> get(const std::string& key) override;
  void put(const std::string& key, HttpResponse response, int ttl) override;
  void remove(const std::string& key) override;

 private:
  // Helpers
  void move_to_head(std::shared_ptr<CacheNode> node);
  void remove_node(std::shared_ptr<CacheNode> node);
  void add_to_head(std::shared_ptr<CacheNode> node);
  std::shared_ptr<CacheNode> remove_tail();

  // Members
  int capacity;                                                          ///< Maximum capacity of the cache
  int size;                                                              ///< Current number of entries in the cache
  std::shared_ptr<CacheNode> head;                                       ///< Dummy head node of the doubly linked list
  std::shared_ptr<CacheNode> tail;                                       ///< Dummy tail node of the doubly linked list
  std::unordered_map<std::string, std::shared_ptr<CacheNode>> cacheMap;  ///< Map for fast O(1) node lookups by key
  std::mutex cacheMutex;                                                 ///< Mutex for thread-safe synchronization
};