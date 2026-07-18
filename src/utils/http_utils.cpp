#include "utils/http_utils.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

#include "config/config.hpp"
#include "error/ErrorHandler.hpp"
#include "network/IConnection.hpp"
#include "vendor/picohttpparser.h"

namespace http {

// Helper Functions

// reads from the socket until the complete http request line and headers are received
static size_t read_header_block(IConnection& socket, std::vector<char>& buffer, const char** method, size_t* method_len, const char** path,
                                size_t* path_len, struct phr_header* headers, size_t* num_headers, int* minor_version_out) {
  size_t prevbuflen = 0;
  int minor_version;

  while (true) {
    char read_buf[4096];
    size_t read_res = 0;
    IoResult io_res = socket.read(read_buf, sizeof(read_buf), read_res);  // fetch data from socket
    if (io_res == IoResult::Closed) {
      throw SocketClosedException();  // exit if connection closed by client
    }
    if (io_res == IoResult::Error) {
      throw ProxyException(400, "Malformed HTTP request headers");
    }

    size_t prev_size = buffer.size();
    buffer.insert(buffer.end(), read_buf, read_buf + read_res);  // append read bytes to buffer

    int res = phr_parse_request(buffer.data(), buffer.size(), method, method_len, path, path_len, &minor_version, headers, num_headers,
                                prevbuflen);  // try parsing the request headers

    if (res > 0) {
      *minor_version_out = minor_version;
      return static_cast<size_t>(res);  // return total header block length
    } else if (res == -1) {
      throw ProxyException(400, "Malformed HTTP request headers");
    } else if (res == -2) {
      prevbuflen = prev_size;  // store progress for partial parsing
      if (buffer.size() >= static_cast<size_t>(config.MAX_HEADER_BYTES)) {
        throw ProxyException(431, "HTTP header size limit exceeded");
      }
    }
  }
}

// reads the remaining payload from the socket to satisfy the content-length
static void read_body_block_unchunked(IConnection& socket, std::vector<char>& buffer, size_t header_len, size_t content_length) {
  while (buffer.size() - header_len < content_length) {  // read until body length is reached
    char read_buf[4096];
    size_t read_res = 0;
    IoResult io_res = socket.read(read_buf, sizeof(read_buf), read_res);
    if (io_res == IoResult::Closed || io_res == IoResult::Error) {
      throw SocketClosedException();  // exit on premature socket shutdown
    }
    buffer.insert(buffer.end(), read_buf, read_buf + read_res);
  }
}

// reads chunked payload from socket until termination chunk is received
static size_t read_body_block_chunked(IConnection& socket, std::vector<char>& buffer, size_t header_len) {
  struct phr_chunked_decoder decoder = {};  // initialize chunk decoder

  while (true) {
    size_t raw_body_size = buffer.size() - header_len;
    size_t decoded_size = raw_body_size;

    ssize_t res = phr_decode_chunked(&decoder, buffer.data() + header_len, &decoded_size);  // decode chunks in-place

    if (res >= 0) {
      buffer.resize(header_len + decoded_size);  // resize buffer to actual decoded payload size
      return decoded_size;
    }
    if (res == -1) {
      throw ProxyException(400, "Malformed chunked body");
    }
    if (res == -2) {
      char read_buf[4096];
      size_t read_res = 0;
      IoResult io_res = socket.read(read_buf, sizeof(read_buf), read_res);  // fetch more chunk data
      if (io_res == IoResult::Closed || io_res == IoResult::Error) {
        throw SocketClosedException();
      }
      buffer.insert(buffer.end(), read_buf, read_buf + read_res);
    }
  }
}

// normalizes absolute paths and strips ports from hostnames
static void normalize_request(HttpRequest& req) {
  // strip absolute uri schemes
  if (req.path.substr(0, 7) == "http://") {
    auto path_start = req.path.find('/', 7);
    req.path = (path_start != std::string::npos) ? req.path.substr(path_start) : "/";
  } else if (req.path.substr(0, 8) == "https://") {
    auto path_start = req.path.find('/', 8);
    req.path = (path_start != std::string::npos) ? req.path.substr(path_start) : "/";
  }

  // remove port from host
  auto colon = req.host.find(':');
  if (colon != std::string::npos) {
    req.host = req.host.substr(0, colon);
  }
}

// converts response to serialization format to send to client
std::string serialize_response(HttpResponse& response) {
  std::string status_msg = get_status_message(response.status_code);
  std::string raw_resp = "HTTP/1.1 " + std::to_string(response.status_code) + " " + status_msg + "\r\n";  // format status line

  // fallback to content-length if missing
  if (response.headers.find("content-length") == response.headers.end() &&
      response.headers.find("transfer-encoding") == response.headers.end()) {
    response.headers["content-length"] = std::to_string(response.body.size());
  }

  for (const auto& [key, value] : response.headers) {
    raw_resp += key + ": " + value + "\r\n";  // format headers
  }

  raw_resp += "\r\n";
  raw_resp += response.body;  // append body
  return raw_resp;
}

// Public Functions
std::string get_status_message(int status_code) {
  switch (status_code) {
    case 200:
      return "OK";
    case 304:
      return "Not Modified";
    case 400:
      return "Bad Request";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 500:
      return "Internal Server Error";
    case 502:
      return "Bad Gateway";
    default:
      return "OK";
  }
}

void read_and_parse_request(HttpContext& ctx) {
  std::vector<char> req_buf;
  const char* method;
  size_t method_len;
  const char* path;
  size_t path_len;
  struct phr_header headers[100];
  size_t num_headers = sizeof(headers) / sizeof(headers[0]);
  int minor_version = 1;

  // read and parse http request line + headers
  size_t header_len =
      read_header_block(ctx.connection(), req_buf, &method, &method_len, &path, &path_len, headers, &num_headers, &minor_version);

  ctx.request.method = std::string(method, method_len);
  ctx.request.path = std::string(path, path_len);
  ctx.request.http_minor = minor_version;

  // extract headers, host, and content-length
  std::string host = "";
  size_t content_length = 0;

  for (size_t i = 0; i < num_headers; ++i) {
    std::string name(headers[i].name, headers[i].name_len);
    std::string value(headers[i].value, headers[i].value_len);

    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });  // lowercase header names

    // append multivalued headers
    auto it = ctx.request.headers.find(name);
    if (it != ctx.request.headers.end()) {
      it->second += ", " + value;
    } else {
      ctx.request.headers[name] = value;
    }

    if (name == "host") {
      host = value;
    } else if (name == "content-length") {
      content_length = std::stoull(value);
    }
  }

  if (host.empty()) {
    throw ProxyException(400, "Malformed HTTP request: missing Host header");
  }
  ctx.request.host = host;

  // read request body if content-length or chunked encoding is present
  if (content_length > 0) {
    read_body_block_unchunked(ctx.connection(), req_buf, header_len, content_length);
    ctx.request.body = std::string(req_buf.data() + header_len, content_length);

  } else if (ctx.request.headers.count("transfer-encoding") > 0 &&
             ctx.request.headers["transfer-encoding"].find("chunked") != std::string::npos) {
    size_t decoded_len = read_body_block_chunked(ctx.connection(), req_buf, header_len);
    ctx.request.body = std::string(req_buf.data() + header_len, decoded_len);
  }

  // normalize path and hostname
  normalize_request(ctx.request);
}

void populate_response(HttpContext& ctx, const httplib::Response& res) {
  size_t content_length = 0;
  if (res.has_header("Content-Length")) {
    content_length = std::stoull(res.get_header_value("Content-Length"));
  }

  // populate response body and status
  ctx.response.status_code = res.status;
  ctx.response.body = res.body;

  // copy headers to ctx.response
  for (const auto& [key, value] : res.headers) {
    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c) { return std::tolower(c); });
    if (k == "content-length" || k == "transfer-encoding") continue;  // skip transmission headers
    auto it = ctx.response.headers.find(k);
    if (it != ctx.response.headers.end()) {
      it->second += ", " + value;
    } else {
      ctx.response.headers[k] = value;
    }
  }

  ctx.response.ttl = 0;
  if (ctx.request.method != "GET" && ctx.request.method != "HEAD") return;  // cache only get/head
  if (ctx.response.status_code != 200) return;
  if (content_length > config.MAX_CACHEABLE) return;  // skip if above size threshold
  if (ctx.response.headers.find("cache-control") == ctx.response.headers.end()) return;

  std::string cc = ctx.response.headers["cache-control"];
  if (cc.find("no-cache") != std::string::npos || cc.find("private") != std::string::npos || cc.find("no-store") != std::string::npos) {
    return;  // respect cache prevention directives
  }

  // parse response ttl from cache-control s-maxage or max-age directives
  int parsed_ttl = 0;
  if (cc.find("s-maxage") != std::string::npos) {
    size_t start = cc.find("s-maxage") + 9;
    size_t end = cc.find(",", start);
    if (end == std::string::npos) {
      end = cc.length();
    }
    parsed_ttl = std::stoi(cc.substr(start, end - start));
  } else if (cc.find("max-age") != std::string::npos) {
    size_t start = cc.find("max-age") + 8;
    size_t end = cc.find(",", start);
    if (end == std::string::npos) {
      end = cc.length();
    }
    parsed_ttl = std::stoi(cc.substr(start, end - start));
  }

  ctx.response.ttl = parsed_ttl;  // save ttl to response metadata
}

void send_response(HttpContext& ctx) {
  if (ctx.response.committed) {
    return;  // prevent duplicate writes to connection
  }
  ctx.response.headers.erase("connection");
  ctx.response.headers.erase("keep-alive");
  ctx.response.headers["connection"] = ctx.keep_alive ? "keep-alive" : "close";
  if (ctx.keep_alive) {
    ctx.response.headers["keep-alive"] =
        "timeout=" + std::to_string(config.KEEPALIVE_IDLE_TIMEOUT) + ", max=" + std::to_string(config.MAX_REQUESTS_PER_CONNECTION);
  }
  std::string raw_resp = serialize_response(ctx.response);   // format downstream response
  ctx.connection().write(raw_resp.data(), raw_resp.size());  // write to connection socket
  ctx.response.committed = true;
}

bool wants_keep_alive(const HttpRequest& req) {
  auto it = req.headers.find("connection");
  if (it != req.headers.end()) {
    std::string v = it->second;
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return std::tolower(c); });
    if (v.find("close") != std::string::npos) return false;
    if (v.find("keep-alive") != std::string::npos) return true;
  }
  return req.http_minor >= 1;  // default to keep-alive on HTTP/1.1 or above
}

}  // namespace http
