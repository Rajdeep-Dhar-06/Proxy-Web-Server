#include "utils/read_parse_request.hpp"
#include <algorithm>
#include <cctype>
#include <vector>
#include "error/ErrorHandler.hpp"
#include "vendor/picohttpparser.h"

void read_and_parse_request(HttpContext& ctx) {
  std::vector<char> req_buf;
  size_t prevbuflen = 0;

  const char* method;
  size_t method_len;
  const char* path;
  size_t path_len;
  int minor_version;
  struct phr_header headers[100];
  size_t num_headers;

  while (true) {
    char read_buf[4096];
    auto read_res = ctx.socket.read(read_buf, sizeof(read_buf));
    if (read_res <= 0) {
      throw SocketClosedException();
    }

    size_t prev_size = req_buf.size();
    req_buf.insert(req_buf.end(), read_buf, read_buf + read_res);

    num_headers = sizeof(headers) / sizeof(headers[0]);
    int res = phr_parse_request(req_buf.data(), req_buf.size(), &method, &method_len, &path, &path_len, &minor_version, headers,
                                &num_headers, prevbuflen);

    if (res > 0) {
      break;  // parsing complete
    } else if (res == -1) {
      throw ProxyException(400, "Bad Request", "Malformed HTTP request");
    } else if (res == -2) {
      prevbuflen = prev_size;
      if (req_buf.size() >= 8192) {
        throw ProxyException(431, "Request Header Fields Too Large", "HTTP header size exceeds 8KB limit");
      }
    }
  }

  const std::string HTTP_SCHEME = "http://";

  ctx.request.method = std::string(method, method_len);
  ctx.request.path = std::string(path, path_len);

  // Extract host
  std::string host = "";
  for (size_t i = 0; i < num_headers; ++i) {
    std::string name(headers[i].name, headers[i].name_len);
    // Convert to Lowercase
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
    if (name == "host") {
      host = std::string(headers[i].value, headers[i].value_len);
      break;
    }
  }

  if (host.empty()) {
    throw ProxyException(400, "Bad Request", "Malformed HTTP request: missing Host header");
  }
  ctx.request.host = host;

  // Clean the path if it is absolute
  if (ctx.request.path.substr(0, HTTP_SCHEME.size()) == HTTP_SCHEME) {
    auto path_start = ctx.request.path.find('/', HTTP_SCHEME.size());
    ctx.request.path = (path_start != std::string::npos) ? ctx.request.path.substr(path_start) : "/";
  }

  // Remove Port from Host if present
  auto colon = ctx.request.host.find(':');
  if (colon != std::string::npos) {
    ctx.request.host = ctx.request.host.substr(0, colon);
  }
}
