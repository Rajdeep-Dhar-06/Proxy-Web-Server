/**
 * @file HTTPParser.hpp
 * @brief Declarations and structures for HTTP request parsing.
 *
 * This file declares the ParsedRequest structure, which holds key components of
 * an incoming HTTP request, and the parse_request function that populates it.
 */

#pragma once
#include <string>

/**
 * @struct ParsedRequest
 * @brief Representation of components extracted from a raw HTTP request.
 *
 * Contains the clean HTTP method, path, and target host of the request.
 */
struct ParsedRequest {
  std::string method; ///< The HTTP method (e.g. "GET", "POST")
  std::string path;   ///< The relative request path (e.g. "/index.html")
  std::string host;   ///< The target host domain name or IP (e.g. "example.com")
};

/**
 * @brief Parses raw HTTP request text into a structured ParsedRequest object.
 *
 * Scans the first line of the request for the method, path, and version,
 * extracts the Host header, and normalizes both the path (making it relative)
 * and the host (stripping port information).
 *
 * @param http_text The raw request string received from the client.
 * @return ParsedRequest The populated struct representing the HTTP request.
 * @throws std::runtime_error if the request is malformed or missing headers.
 */
ParsedRequest parse_request(const std::string &http_text);
