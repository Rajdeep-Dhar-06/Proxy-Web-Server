#pragma once
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

using namespace std;

struct CacheNode {
  string url;
  string response;
  CacheNode *prev;
  CacheNode *next;

  CacheNode(const string &key, string val)
      : url(key), response(std::move(val)), prev(nullptr), next(nullptr) {}
};

class LRUCache {
private:
  int capacity;
  int size;
  CacheNode *head;
  CacheNode *tail;
  unordered_map<string, CacheNode *> cacheMap;
  mutex cacheMutex;

  void moveToHead(CacheNode *node);
  void removeNode(CacheNode *node);
  void addToHead(CacheNode *node);
  CacheNode *removeTail();

public:
  LRUCache(int cap);
  ~LRUCache();

  optional<string> get(const string &url);
  void put(const string &url, string response);
};