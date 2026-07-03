#include "utils/http_utils.hpp"

#include <sockpp/tcp_socket.h>

#include <algorithm>
#include <cctype>
#include <vector>

#include "config/config.hpp"
#include "error/ErrorHandler.hpp"
#include "vendor/picohttpparser.h"

namespace http {

// Helper Functions

// Reads from the socket until the complete HTTP request line and headers are received
static size_t read_header_block(sockpp::tcp_socket& socket, std::vector<char>& buffer, const char** method, size_t* method_len,
                                const char** path, size_t* path_len, struct phr_header* headers, size_t* num_headers) {
  size_t prevbuflen = 0;
  int minor_version;

  while (true) {
    char read_buf[4096];
    auto read_res = socket.read(read_buf, sizeof(read_buf));
    if (read_res <= 0) {
      throw SocketClosedException();
    }

    size_t prev_size = buffer.size();
    buffer.insert(buffer.end(), read_buf, read_buf + read_res);

    int res = phr_parse_request(buffer.data(), buffer.size(), method, method_len, path, path_len, &minor_version, headers, num_headers,
                                prevbuflen);

    if (res > 0) {
      return static_cast<size_t>(res);  // Return header block length
    } else if (res == -1) {
      throw ProxyException(400, "Bad Request", "Malformed HTTP request headers");
    } else if (res == -2) {
      prevbuflen = prev_size;
      if (buffer.size() >= static_cast<size_t>(config.MAX_HEADER_BYTES)) {
        throw ProxyException(431, "Request Header Fields Too Large", "HTTP header size limit exceeded");
      }
    }
  }
}

// Reads the remaining payload from the socket to satisfy the Content-Length
static void read_body_block(sockpp::tcp_socket& socket, std::vector<char>& buffer, size_t header_len, size_t content_length) {
  while (buffer.size() - header_len < content_length) {
    char read_buf[4096];
    auto read_res = socket.read(read_buf, sizeof(read_buf));
    if (read_res <= 0) {
      throw SocketClosedException();
    }
    buffer.insert(buffer.end(), read_buf, read_buf + read_res);
  }
}

// Normalizes absolute paths and strips ports from hostnames
static void normalize_request(HttpRequest& req) {
  // Strip absolute URI schemes (http:// and https://)
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

// Converts response to send to client
std::string serialize_response(HttpResponse& response) {
  std::string status_msg = get_status_message(response.status_code);
  std::string raw_resp = "HTTP/1.1 " + std::to_string(response.status_code) + " " + status_msg + "\r\n";

  if (response.headers.find("Content-Length") == response.headers.end() &&
      response.headers.find("Transfer-Encoding") == response.headers.end()) {
    response.headers["Content-Length"] = std::to_string(response.body.size());
  }

  for (const auto& [key, value] : response.headers) {
    raw_resp += key + ": " + value + "\r\n";
  }

  raw_resp += "\r\n";
  raw_resp += response.body;
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

  // 1. Read and parse HTTP Request Line + Headers
  size_t header_len = read_header_block(ctx.socket, req_buf, &method, &method_len, &path, &path_len, headers, &num_headers);

  ctx.request.method = std::string(method, method_len);
  ctx.request.path = std::string(path, path_len);

  // 2. Extract headers, Host, and Content-Length
  std::string host = "";
  size_t content_length = 0;

  for (size_t i = 0; i < num_headers; ++i) {
    std::string name(headers[i].name, headers[i].name_len);
    std::string value(headers[i].value, headers[i].value_len);

    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
    ctx.request.headers[name] = value;

    if (name == "host") {
      host = value;
    } else if (name == "content-length") {
      content_length = std::stoull(value);
    }
  }

  if (host.empty()) {
    throw ProxyException(400, "Bad Request", "Malformed HTTP request: missing Host header");
  }
  ctx.request.host = host;

  // 3. Read request body if present
  if (content_length > 0) {
    read_body_block(ctx.socket, req_buf, header_len, content_length);
    ctx.request.body = std::string(req_buf.data() + header_len, content_length);
  }

  // 4. Normalize paths and host headers
  normalize_request(ctx.request);
}

void populate_response(HttpContext& ctx, const httplib::Response& res) {
  size_t content_length = 0;
  if (res.has_header("Content-Length")) {
    content_length = std::stoull(res.get_header_value("Content-Length"));
  }

  bool exceeds_cacheable = (content_length > config.MAX_CACHEABLE);

  // 1. Populate HttpResponse inside ctx
  ctx.response.status_code = res.status;
  ctx.response.body = res.body;

  // Only cache successful 200 OK responses
  ctx.response.ttl = (res.status == 200 && !exceeds_cacheable) ? config.TTL : 0;

  // 2. Parse Cache-Control TTL if it is a 200 response and within cacheable limits
  if (res.status == 200 && !exceeds_cacheable && res.has_header("Cache-Control")) {
    std::string cache_control = res.get_header_value("Cache-Control");

    if (cache_control.find("no-store") != std::string::npos || cache_control.find("no-cache") != std::string::npos ||
        cache_control.find("private") != std::string::npos) {
      // Force minimum TTL even if told not to cache
      ctx.response.ttl = config.TTL;
    } else {
      size_t pos = cache_control.find("max-age=");
      if (pos != std::string::npos) {
        std::string num_str = cache_control.substr(pos + 8);
        size_t comma_pos = num_str.find(',');
        if (comma_pos != std::string::npos) {
          num_str = num_str.substr(0, comma_pos);
        }
        try {
          int max_age = std::stoi(num_str);
          // Enforce minimum TTL of config.TTL (60s)
          ctx.response.ttl = std::max(max_age, config.TTL);
        } catch (...) {
          ctx.response.ttl = config.TTL;
        }
      }
    }
  }

  // 3. Forward safe headers from the origin response
  for (const auto& [key, value] : res.headers) {
    if (key == "Content-Length" || key == "Transfer-Encoding") continue;
    ctx.response.headers[key] = value;
  }
}

void send_response(HttpContext& ctx) {
  if (ctx.response.committed) {
    return;
  }
  std::string raw_resp = serialize_response(ctx.response);
  ctx.socket.write_n(raw_resp.data(), raw_resp.size());
  ctx.response.committed = true;
}

}  // namespace http
