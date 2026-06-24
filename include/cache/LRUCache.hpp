/**
 * @file LRUCache.hpp
 * @brief Thread-safe Least Recently Used (LRU) Cache implementation.
 *
 * This file contains the declaration of LRUCache and CacheNode. It implements
 * a classic doubly linked list combined with an unordered map to achieve O(1)
 * lookups and updates while keeping track of access order for eviction.
 */

#pragma once
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <chrono>

using timepoint = std::chrono::steady_clock::time_point;

/**
 * @struct CacheNode
 * @brief Node representation in the LRUCache doubly linked list.
 *
 * Represents an individual entry in the cache, holding the request URL as
 * the key, the raw HTTP response as the value, and pointers to adjacent nodes.
 */
struct CacheNode {
  std::string url;         ///< Cached URL key
  std::string response;    ///< Raw HTTP response value
  CacheNode *prev;         ///< Pointer to the previous node in the list
  CacheNode *next;         ///< Pointer to the next node in the list
  timepoint expiration;    ///< Expiration Time

  /**
   * @brief Constructs a new CacheNode.
   * @param key The cache key (URL).
   * @param val The cache value (response payload).
   */
  CacheNode(const std::string &key, std::string val, timepoint expires_at)
      : url(key), response(std::move(val)), prev(nullptr), next(nullptr), expiration(expires_at) {}
};

/**
 * @class LRUCache
 * @brief A thread-safe Least Recently Used (LRU) cache.
 *
 * Manages cache entries with a capacity constraint. When the cache exceeds its
 * capacity, it evicts the least recently accessed items first. All public methods
 * are synchronized using a mutex to ensure thread-safety.
 */
class LRUCache {
private:
  int capacity;                                 ///< Maximum capacity of the cache
  int size;                                     ///< Current number of entries in the cache
  CacheNode *head;                              ///< Dummy head node of the doubly linked list
  CacheNode *tail;                              ///< Dummy tail node of the doubly linked list
  std::unordered_map<std::string, CacheNode *> cacheMap;  ///< Map for fast O(1) node lookups by URL
  std::mutex cacheMutex;                             ///< Mutex for thread-safe synchronization

  /**
   * @brief Promotes a node to the head of the doubly linked list (most recently used).
   * @param node Pointer to the node to move.
   */
  void moveToHead(CacheNode *node);

  /**
   * @brief Unlinks a node from its neighbors in the doubly linked list.
   * @param node Pointer to the node to remove.
   */
  void removeNode(CacheNode *node);

  /**
   * @brief Inserts a node immediately after the dummy head node.
   * @param node Pointer to the node to insert.
   */
  void addToHead(CacheNode *node);

  /**
   * @brief Removes the least recently used node (immediately before dummy tail).
   * @return CacheNode* Pointer to the removed node. Caller gains ownership of the memory.
   */
  CacheNode *removeTail();

public:
  /**
   * @brief Constructs an LRUCache with a specified capacity.
   * @param cap The maximum number of elements the cache can hold.
   */
  LRUCache(int cap);

  /**
   * @brief Destructor that cleans up all dynamically allocated CacheNodes.
   */
  ~LRUCache();

  /**
   * @brief Retrieves a response from the cache.
   *
   * If the key exists, updates the item's access order to make it most recently
   * used, and returns the response. Otherwise, returns std::nullopt.
   *
   * @param url The request URL key.
   * @return std::optional<string> The cached response if found, else std::nullopt.
   */
  std::optional<std::string> get(const std::string &url);

  /**
   * @brief Stores a response in the cache.
   *
   * If the key already exists, updates the value and moves the node to the head.
   * If it's a new key, creates a new node, inserts it at the head, and evicts
   * the least recently used node if the capacity limit is exceeded.
   *
   * @param url The request URL key.
   * @param response The response payload to cache.
   */
  void put(const std::string &url, std::string response, int ttl);
};