#include <gtest/gtest.h>
#include <cstring>
#include <algorithm>
#include "utils/http_utils.hpp"
#include "error/ErrorHandler.hpp"

class MockConnection : public IConnection {
public:
  MockConnection(std::string data) : data_(std::move(data)), offset_(0) {}

  IoResult read(char* buf, size_t len, size_t& bytes_read) override {
    if (offset_ >= data_.size()) {
      bytes_read = 0;
      return IoResult::Closed;
    }
    size_t to_read = std::min(len, data_.size() - offset_);
    std::memcpy(buf, data_.data() + offset_, to_read);
    offset_ += to_read;
    bytes_read = to_read;
    return IoResult::Ok;
  }

  IoResult write(const char* buf, size_t len) override {
    written_.append(buf, len);
    return IoResult::Ok;
  }

  void close() override {}

  std::string data_;
  size_t offset_;
  std::string written_;
};

TEST(HttpParserTest, ParseValidGetRequest) {
  std::string raw_req = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
  auto conn = std::make_unique<MockConnection>(raw_req);
  HttpContext ctx(std::move(conn));
  
  http::read_and_parse_request(ctx);
  
  EXPECT_EQ(ctx.request.method, "GET");
  EXPECT_EQ(ctx.request.path, "/index.html");
  EXPECT_EQ(ctx.request.host, "localhost");
}

TEST(HttpParserTest, ParseMalformedRequestThrows) {
  std::string raw_req = "GET /index.html\r\nHost: localhost\r\n\r\n";
  auto conn = std::make_unique<MockConnection>(raw_req);
  HttpContext ctx(std::move(conn));
  
  EXPECT_THROW(http::read_and_parse_request(ctx), ProxyException);
}
