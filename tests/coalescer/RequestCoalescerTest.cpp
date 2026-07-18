#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "network/RequestCoalescer.hpp"

TEST(RequestCoalescerTest, CoalescingBehavior) {
  RequestCoalescer coalescer;
  std::string key = "http://localhost/test";
  
  // Thread 1 starts request
  auto ticket1 = coalescer.start_or_join(key);
  EXPECT_TRUE(ticket1.is_owner);
  
  // Thread 2 joins in-flight request
  auto ticket2 = coalescer.start_or_join(key);
  EXPECT_FALSE(ticket2.is_owner);
  
  // Owner completes request
  auto expected_res = std::make_shared<HttpResponse>();
  expected_res->status_code = 200;
  expected_res->body = "coalesced_data";
  
  coalescer.complete(key, expected_res);
  
  // Both tickets should resolve to the same response
  auto res1 = ticket1.done.get();
  auto res2 = ticket2.done.get();
  
  ASSERT_NE(res1, nullptr);
  EXPECT_EQ(res1->body, "coalesced_data");
  EXPECT_EQ(res1, res2); // Assert it's the exact same shared object
}
