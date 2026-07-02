#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "./LRUCache.hpp"

class ShardedCache : public ICache {
 public:
  // Constructor
  ShardedCache(int total_capacity, int shards_count);

  // Cache operations
  std::optional<HttpResponse> get(const std::string& key);
  void put(const std::string& key, HttpResponse response, int ttl = 3600);

 private:
  // Helpers
  int get_shard_index(const std::string& key);

  // Members
  std::vector<std::unique_ptr<LRUCache>> shards;
  std::hash<std::string> hasher;
  int num_shards;
};