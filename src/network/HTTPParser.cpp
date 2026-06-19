#include "HTTPParser.hpp"

#include <stdexcept>

ParsedRequest parse_request(const std::string& http_text) {
  ParsedRequest req;
  const std::string HTTP_SCHEME = "http://";
  const std::string HOST_MARKER = "\r\nHost: ";

  // =========================================================================
  // STEP 1: Extract Method and Path from the HTTP Request Line
  //
  // Description: Locate the spaces separating the Method, URI, and HTTP Version on the first line of the raw request.   
  //
  // Input:
  //   http_text = "GET http://dummyjson.com/products HTTP/1.1\r\nHost: ..."
  //
  // Outputs:
  //   req.method = "GET"
  //   req.path   = "http://dummyjson.com/products"
  // =========================================================================

  auto space1 = http_text.find(' ');
  auto space2 = http_text.find(' ', space1 + 1);

  if (space1 == std::string::npos || space2 == std::string::npos) throw std::runtime_error("Malformed HTTP request: bad request line");

  req.method = http_text.substr(0, space1);
  req.path = http_text.substr(space1 + 1, space2 - space1 - 1);

  // =========================================================================
  // STEP 2: Clean the Request Path
  //
  // Description: Clients send absolute URLs in the request line. Upstream origin servers expect a relative path.
  //              If the path starts with "http://", strip the protocol and domain.
  //
  // Input:
  //   req.path = "http://dummyjson.com/products"
  //
  // Output:
  //   req.path = "/products"
  // =========================================================================

  if (req.path.substr(0, HTTP_SCHEME.size()) == HTTP_SCHEME) {
    auto path_start = req.path.find('/', HTTP_SCHEME.size());
    req.path = (path_start != std::string::npos) ? req.path.substr(path_start) : "/";
  }

  // =========================================================================
  // STEP 3: Extract the Host Header
  //
  // Description: Locate the "Host: " header in the HTTP header block.
  //
  // Input:
  //   http_text = "...\r\nHost: dummyjson.com:8080\r\n..."
  //
  // Output:
  //   req.host = "dummyjson.com:8080"
  // =========================================================================

  size_t host_pos = http_text.find(HOST_MARKER);
  if (host_pos == std::string::npos) throw std::runtime_error("Malformed HTTP request: missing Host header");

  size_t host_start = host_pos + HOST_MARKER.size();
  auto host_end = http_text.find("\r\n", host_start);

  req.host = http_text.substr(host_start, host_end - host_start);

  // =========================================================================
  // STEP 4: Remove Port from Host
  //
  // Description: If the host header includes a port suffix (e.g., :8080), strip it.
  //
  // Input:
  //   req.host = "dummyjson.com:8080"
  //
  // Output:
  //   req.host = "dummyjson.com"
  // =========================================================================

  auto colon = req.host.find(':');
  if (colon != std::string::npos) req.host = req.host.substr(0, colon);

  return req;
}
