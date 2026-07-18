#include "cache/ShardedCache.hpp"

size_t ShardedCache::get_shard_index(const std::string& key) {
  return hasher(key) % num_shards;
}

ShardedCache::ShardedCache(size_t total_capacity, size_t total_shards) : num_shards(total_shards) {
  shards.reserve(num_shards);

  size_t capacity_per_shard = total_capacity / num_shards;
  for (size_t i = 0; i < num_shards; i++) {
    shards.push_back(std::make_unique<LRUCache>(capacity_per_shard));
  }
}

std::optional<HttpResponse> ShardedCache::get(const std::string& key) {
  return shards[get_shard_index(key)]->get(key);
}

void ShardedCache::put(const std::string& key, HttpResponse response, int ttl) {
  shards[get_shard_index(key)]->put(key, std::move(response), ttl);
}

void ShardedCache::remove(const std::string& key) { shards[get_shard_index(key)]->remove(key); }