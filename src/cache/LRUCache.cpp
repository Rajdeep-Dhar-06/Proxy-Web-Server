#include "cache/LRUCache.hpp"

#include <chrono>
#include <memory>
#include <optional>

LRUCache::LRUCache(size_t cap) {
  capacity = cap;
  size = 0;
  // Initialize dummy head and tail nodes to avoid checking for null pointers in list operations
  head = std::make_shared<CacheNode>("", HttpResponse{}, std::chrono::steady_clock::time_point::max());
  tail = std::make_shared<CacheNode>("", HttpResponse{}, std::chrono::steady_clock::time_point::max());
  head->next = tail;
  tail->prev = head;
}

LRUCache::~LRUCache() {
  std::shared_ptr<CacheNode> curr = head;
  while (curr) {  // Converting Recursive Destruction to Iterative
    std::shared_ptr<CacheNode> nextNode = curr->next;
    curr->next.reset();
    curr = nextNode;
  }
}

void LRUCache::remove_node(std::shared_ptr<CacheNode> node) {
  // Unlink the node from its current position in the list
  if (!node) return;
  std::shared_ptr<CacheNode> prevNode = node->prev.lock();
  if (prevNode) {
    prevNode->next = node->next;
  }
  if (node->next) {
    node->next->prev = prevNode;
  }
  node->next.reset();
  node->prev.reset();
}

void LRUCache::add_to_head(std::shared_ptr<CacheNode> node) {
  // Insert the node immediately after the dummy head node
  node->prev = head;
  node->next = head->next;
  head->next->prev = node;
  head->next = node;
}

void LRUCache::move_to_head(std::shared_ptr<CacheNode> node) {
  remove_node(node);
  add_to_head(node);
}

std::shared_ptr<CacheNode> LRUCache::remove_tail() {
  // The actual LRU node is the one right before the dummy tail node
  std::shared_ptr<CacheNode> tailNode = tail->prev.lock();
  remove_node(tailNode);
  return tailNode;
}

std::optional<HttpResponse> LRUCache::get(const std::string& key) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  auto it = cacheMap.find(key);
  if (it != cacheMap.end()) {
    std::shared_ptr<CacheNode> foundNode = it->second;

    auto now = std::chrono::steady_clock::now();
    if (now >= foundNode->expiration) {
      remove_node(foundNode);
      cacheMap.erase(key);
      return std::nullopt;
    }

    move_to_head(foundNode);  // Access promotes the node to MRU status
    return foundNode->response;
  }
  return std::nullopt;
}

void LRUCache::put(const std::string& key, HttpResponse response, int ttl) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  auto now = std::chrono::steady_clock::now();
  auto expires_at = now + std::chrono::seconds(ttl);

  // Case 1: The key is already present in the cache; update response and promote node
  auto it = cacheMap.find(key);
  if (it != cacheMap.end()) {
    std::shared_ptr<CacheNode> existingNode = it->second;
    existingNode->response = std::move(response);
    existingNode->expiration = expires_at;
    move_to_head(existingNode);
    return;
  }

  // Case 2: The key is new. Evict the LRU node first if we are at capacity
  if (size >= capacity) {
    std::shared_ptr<CacheNode> lruNode = remove_tail();
    cacheMap.erase(lruNode->key);
    size--;
  }

  // Insert the new node at the head
  std::shared_ptr<CacheNode> newNode = std::make_shared<CacheNode>(key, std::move(response), expires_at);
  add_to_head(newNode);
  cacheMap[key] = newNode;
  size++;
}