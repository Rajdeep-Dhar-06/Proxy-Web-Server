#include <gtest/gtest.h>
#include "cache/LRUCache.hpp"

TEST(LRUCacheTest, BasicPutAndGet) {
  LRUCache cache(2);
  
  HttpResponse res1;
  res1.status_code = 200;
  res1.body = "Hello";
  
  cache.put("key1", res1, 10);
  auto val = cache.get("key1");
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(val->body, "Hello");
}

TEST(LRUCacheTest, EvictionLogic) {
  LRUCache cache(2);
  HttpResponse res;
  res.status_code = 200;
  res.body = "data";
  
  cache.put("key1", res, 10);
  cache.put("key2", res, 10);
  cache.put("key3", res, 10); // key1 should be evicted since capacity is 2
  
  EXPECT_FALSE(cache.get("key1").has_value());
  EXPECT_TRUE(cache.get("key2").has_value());
  EXPECT_TRUE(cache.get("key3").has_value());
}
