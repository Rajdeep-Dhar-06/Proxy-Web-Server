#include "LRUCache.hpp"

LRUCache::LRUCache(int cap) {
  capacity = cap;
  size = 0;
  head = new CacheNode("", "");
  tail = new CacheNode("", "");
  head->next = tail;
  tail->prev = head;
}

LRUCache::~LRUCache() {
  CacheNode *curr = head;
  while (curr != nullptr) {
    CacheNode *nextNode = curr->next;
    delete curr;
    curr = nextNode;
  }
}

void LRUCache::removeNode(CacheNode *node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
}

void LRUCache::addToHead(CacheNode *node) {
  node->prev = head;
  node->next = head->next;
  head->next->prev = node;
  head->next = node;
}

void LRUCache::moveToHead(CacheNode *node) {
  removeNode(node);
  addToHead(node);
}

CacheNode *LRUCache::removeTail() {
  CacheNode *tailNode = tail->prev;
  removeNode(tailNode);
  return tailNode;
}

optional<string> LRUCache::get(const string &url) {
  lock_guard<mutex> lock(cacheMutex);
  auto it = cacheMap.find(url);
  if (it != cacheMap.end()) {
    CacheNode *foundNode = it->second;
    moveToHead(foundNode);
    return foundNode->response;
  }
  return nullopt;
}

void LRUCache::put(const string &url, string response) {
  lock_guard<mutex> lock(cacheMutex);

  // 1. The URL already in the cache
  auto it = cacheMap.find(url);
  if (it != cacheMap.end()) {
    CacheNode *existingNode = it->second;
    existingNode->response = std::move(response);
    moveToHead(existingNode);
    return;
  }

  // 2. The URL is new
  if (size >= capacity) {
    CacheNode *lruNode = removeTail();
    cacheMap.erase(lruNode->url);
    delete lruNode;
    size--;
  }

  CacheNode *newNode = new CacheNode(url, std::move(response));
  addToHead(newNode);
  cacheMap[url] = newNode;
  size++;
}