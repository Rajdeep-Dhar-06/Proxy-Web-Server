#include "cache/LRUCache.hpp"

#include <chrono>
#include <optional>

LRUCache::LRUCache(int cap) {
  capacity = cap;
  size = 0;
  // Initialize dummy head and tail nodes to avoid checking for null pointers in list operations
  head = new CacheNode("", "", std::chrono::steady_clock::time_point::max());
  tail = new CacheNode("", "", std::chrono::steady_clock::time_point::max());
  head->next = tail;
  tail->prev = head;
}

LRUCache::~LRUCache() {
  CacheNode* curr = head;
  while (curr != nullptr) {
    CacheNode* nextNode = curr->next;
    delete curr;
    curr = nextNode;
  }
}

void LRUCache::removeNode(CacheNode* node) {
  // Unlink the node from its current position in the list
  node->prev->next = node->next;
  node->next->prev = node->prev;
}

void LRUCache::addToHead(CacheNode* node) {
  // Insert the node immediately after the dummy head node
  node->prev = head;
  node->next = head->next;
  head->next->prev = node;
  head->next = node;
}

void LRUCache::moveToHead(CacheNode* node) {
  removeNode(node);
  addToHead(node);
}

CacheNode* LRUCache::removeTail() {
  // The actual LRU node is the one right before the dummy tail node
  CacheNode* tailNode = tail->prev;
  removeNode(tailNode);
  return tailNode;
}

std::optional<std::string> LRUCache::get(const std::string& url) {
  std::lock_guard<std::mutex> lock(cacheMutex);
  auto it = cacheMap.find(url);
  if (it != cacheMap.end()) {
    CacheNode* foundNode = it->second;

    auto now = std::chrono::steady_clock::now();
    if (now >= foundNode->expiration) {
      removeNode(foundNode);
      cacheMap.erase(url);
      delete foundNode;
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
    CacheNode* existingNode = it->second;
    existingNode->response = std::move(response);
    existingNode->expiration = expires_at;
    moveToHead(existingNode);
    return;
  }

  // Case 2: The URL is new. Evict the LRU node first if we are at capacity
  if (size >= capacity) {
    CacheNode* lruNode = removeTail();
    cacheMap.erase(lruNode->url);
    delete lruNode;
    size--;
  }

  // Insert the new node at the head
  CacheNode* newNode = new CacheNode(url, std::move(response), expires_at);
  addToHead(newNode);
  cacheMap[url] = newNode;
  size++;
}