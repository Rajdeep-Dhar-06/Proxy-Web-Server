#include "ShardedCache.hpp"

int ShardedCache::getShardIndex(const string& url) { return hasher(url) % num_shards; }

ShardedCache::ShardedCache(int total_capacity, int shards_count) : num_shards(shards_count) {
  shards.reserve(num_shards);
  int capacity_per_shard = total_capacity / num_shards;
  for (int i = 0; i < num_shards; i++) {
    shards.push_back(std::make_unique<LRUCache>(capacity_per_shard));
  }
}

optional<string> ShardedCache::get(const string& url) { return shards[getShardIndex(url)]->get(url); }

void ShardedCache::put(const string& url, string response) { shards[getShardIndex(url)]->put(url, std::move(response)); }
