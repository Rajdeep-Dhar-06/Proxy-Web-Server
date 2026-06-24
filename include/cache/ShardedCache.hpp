/**
 * @file ShardedCache.hpp
 * @brief Thread-safe Sharded Cache wrapper.
 *
 * Implements a cache partitioning scheme (sharding) to reduce lock contention
 * under highly concurrent workloads. Requests are routed to individual LRU
 * shards based on the hash of the resource URL.
 */

#pragma once
#include "cache/LRUCache.hpp"
#include <functional>
#include <memory>
#include <vector>

/**
 * @class ShardedCache
 * @brief Thread-safe cache partitioned into multiple independent LRUCache shards.
 *
 * Distributes cached items across a set of shards using a hash function. Each shard
 * contains its own independent lock, enabling threads to access and update cache
 * entries in parallel as long as their hash keys map to different shards.
 */
class ShardedCache {
private:
  vector<std::unique_ptr<LRUCache>> shards;     ///< List of independent LRU cache shards
  hash<string> hasher;                          ///< Hash function to map keys to shards
  int num_shards;                               ///< Total number of shards configured

  /**
   * @brief Helper to compute the shard index for a given URL.
   * @param url The resource URL string.
   * @return int The index of the shard in the shards array.
   */
  int getShardIndex(const string &url);

public:
  /**
   * @brief Constructs a ShardedCache.
   * @param total_capacity The total capacity across all shards combined.
   * @param shards_count The number of partition shards to initialize.
   */
  ShardedCache(int total_capacity, int shards_count);

  /**
   * @brief Retrieves a response from the appropriate cache shard.
   * @param url The resource URL.
   * @return std::optional<string> The cached response if found, else std::nullopt.
   */
  optional<string> get(const string &url);

  /**
   * @brief Stores a response in the appropriate cache shard.
   * @param url The resource URL.
   * @param response The response body to cache.
   */
  void put(const string &url, string response, int ttl);
};