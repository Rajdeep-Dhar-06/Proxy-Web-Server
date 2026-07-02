#include "cache/ShardedCache.hpp"

int ShardedCache::get_shard_index(const std::string& key) {
  // Map the hash of the key to one of the configured shard buckets
  return hasher(key) % num_shards;
}

ShardedCache::ShardedCache(int total_capacity, int shards_count) : num_shards(shards_count) {
  shards.reserve(num_shards);

  // Evenly distribute total capacity capacity among all shard buckets
  int capacity_per_shard = total_capacity / num_shards;
  for (int i = 0; i < num_shards; i++) {
    shards.push_back(std::make_unique<LRUCache>(capacity_per_shard));
  }
}

std::optional<HttpResponse> ShardedCache::get(const std::string& key) {
  // Delegate the lookup request to the specific shard handling this key
  return shards[get_shard_index(key)]->get(key);
}

void ShardedCache::put(const std::string& key, HttpResponse response, int ttl) {
  // Delegate the storage request to the specific shard handling this key
  shards[get_shard_index(key)]->put(key, std::move(response), ttl);
}
