#include "cache/LRUCache.hpp"

#include <chrono>
#include <optional>

LRUCache::LRUCache(size_t cache_capacity) : capacity(cache_capacity) {}

std::optional<HttpResponse> LRUCache::get(const std::string& key) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  auto it = cacheMap.find(key);
  if (it == cacheMap.end()) {
    return std::nullopt;
  }
  auto now = std::chrono::steady_clock::now();
  if (now >= it->second->expiration) {
    cacheList.erase(it->second);
    cacheMap.erase(it);
    return std::nullopt;
  }
  cacheList.splice(cacheList.begin(), cacheList, it->second);
  return it->second->response;
}

void LRUCache::put(const std::string& key, HttpResponse response, int ttl) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  auto now = std::chrono::steady_clock::now();
  auto expires_at = now + std::chrono::seconds(ttl);

  // Case 1: The key is already present in the cache; update response and promote node
  auto it = cacheMap.find(key);
  if (it != cacheMap.end()) {
    it->second->response = std::move(response);
    it->second->expiration = expires_at;
    cacheList.splice(cacheList.begin(), cacheList, it->second);
    return;
  }

  // Case 2: The key is new. Evict the LRU node first if we are at capacity
  if (cacheList.size() >= capacity) {
    cacheMap.erase(cacheList.back().key);
    cacheList.pop_back();
  }

  // Insert the new node at the MRU position
  cacheList.emplace_front(key, std::move(response), expires_at);
  cacheMap[key] = cacheList.begin();
}

void LRUCache::remove(const std::string& key) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  auto it = cacheMap.find(key);

  if (it != cacheMap.end()) {
    cacheList.erase(it->second);
    cacheMap.erase(it);
  }
}
