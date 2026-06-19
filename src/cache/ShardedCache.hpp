#pragma once
#include "LRUCache.hpp"
#include <functional>
#include <memory>
#include <vector>

class ShardedCache {
private:
  vector<std::unique_ptr<LRUCache>> shards;
  int num_shards;
  hash<string> hasher;

  int getShardIndex(const string &url);

public:
  ShardedCache(int total_capacity, int shards_count);
  optional<string> get(const string &url);
  void put(const string &url, string response);
};