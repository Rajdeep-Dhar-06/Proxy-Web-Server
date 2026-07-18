#include <gtest/gtest.h>
#include "cache/ShardedCache.hpp"

TEST(ShardedCacheTest, ShardingAndPutGet) {
  ShardedCache cache(4, 2); // capacity=4, shards=2
  HttpResponse res;
  res.status_code = 200;
  res.body = "Sharded";
  
  cache.put("my_key", res, 10);
  auto val = cache.get("my_key");
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(val->body, "Sharded");
}
