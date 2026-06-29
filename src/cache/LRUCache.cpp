#include "cache/LRUCache.hpp"

#include <chrono>
#include <memory>
#include <optional>

LRUCache::LRUCache(int cap) {
  capacity = cap;
  size = 0;
  // Initialize dummy head and tail nodes to avoid checking for null pointers in list operations
  head = std::make_shared<CacheNode>("", "", std::chrono::steady_clock::time_point::max());
  tail = std::make_shared<CacheNode>("", "", std::chrono::steady_clock::time_point::max());
  head->next = tail;
  tail->prev = head;
}

LRUCache::~LRUCache() {
  std::shared_ptr<CacheNode> curr = head;
  while (curr) { // Converting Recursive Destruction to Iterative
    std::shared_ptr<CacheNode> nextNode = curr->next;
    curr->next.reset();
    curr = nextNode;
  }
}

void LRUCache::removeNode(std::shared_ptr<CacheNode> node) {
  // Unlink the node from its current position in the list
  if (!node) return;
  std::shared_ptr<CacheNode> prevNode = node->prev.lock();
  if (prevNode) {
    prevNode->next = node->next;
  }
  node->next->prev = prevNode;
  node->next.reset();
  node->prev.reset();
}

void LRUCache::addToHead(std::shared_ptr<CacheNode> node) {
  // Insert the node immediately after the dummy head node
  node->prev = head;
  node->next = head->next;
  head->next->prev = node;
  head->next = node;
}

void LRUCache::moveToHead(std::shared_ptr<CacheNode> node) {
  removeNode(node);
  addToHead(node);
}

std::shared_ptr<CacheNode> LRUCache::removeTail() {
  // The actual LRU node is the one right before the dummy tail node
  std::shared_ptr<CacheNode> tailNode = tail->prev.lock();
  removeNode(tailNode);
  return tailNode;
}

std::optional<std::string> LRUCache::get(const std::string& url) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  auto it = cacheMap.find(url);
  if (it != cacheMap.end()) {
    std::shared_ptr<CacheNode> foundNode = it->second;

    auto now = std::chrono::steady_clock::now();
    if (now >= foundNode->expiration) {
      removeNode(foundNode);
      cacheMap.erase(url);
      return std::nullopt;
    }

    moveToHead(foundNode);  // Access promotes the node to MRU status
    return foundNode->response;
  }
  return std::nullopt;
}

void LRUCache::put(const std::string& url, std::string response, int ttl) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  auto now = std::chrono::steady_clock::now();
  auto expires_at = now + std::chrono::seconds(ttl);

  // Case 1: The URL is already present in the cache; update response and promote node
  auto it = cacheMap.find(url);
  if (it != cacheMap.end()) {
    std::shared_ptr<CacheNode> existingNode = it->second;
    existingNode->response = std::move(response);
    existingNode->expiration = expires_at;
    moveToHead(existingNode);
    return;
  }

  // Case 2: The URL is new. Evict the LRU node first if we are at capacity
  if (size >= capacity) {
    std::shared_ptr<CacheNode> lruNode = removeTail();
    cacheMap.erase(lruNode->url);
    size--;
  }

  // Insert the new node at the head
  std::shared_ptr<CacheNode> newNode = std::make_shared<CacheNode>(url, std::move(response), expires_at);
  addToHead(newNode);
  cacheMap[url] = newNode;
  size++;
}