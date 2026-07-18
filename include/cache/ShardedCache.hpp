#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "./LRUCache.hpp"

class ShardedCache : public ICache {
 public:
  // Constructor
  ShardedCache(size_t total_capacity, size_t total_shards);

  // Cache operations
  std::optional<HttpResponse> get(const std::string& key) override;
  void put(const std::string& key, HttpResponse response, int ttl) override;
  void remove(const std::string& key) override;

 private:
  // Helpers
  size_t get_shard_index(const std::string& key);

  // Members
  std::vector<std::unique_ptr<LRUCache>> shards;
  std::hash<std::string> hasher;
  size_t num_shards;
};