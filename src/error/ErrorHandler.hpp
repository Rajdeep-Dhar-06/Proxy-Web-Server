/**
 * @file ErrorHandler.hpp
 * @brief Declarations for custom proxy exceptions and HTTP error generation.
 *
 * This file declares the ProxyException class, which is used to signal proxy-specific
 * errors (such as backend origin unreachable, gateway timeouts, or parsing errors)
 * and generate structured HTTP responses for clients.
 */

#pragma once
#include <exception>
#include <string>

/**
 * @class ProxyException
 * @brief Custom exception class for handling proxy and HTTP gateway errors.
 *
 * Inherits from std::exception. It wraps HTTP status codes, standard HTTP
 * reason phrases, and detailed internal error logs. Provides static helpers
 * to build response payloads directly.
 */
class ProxyException : public std::exception {
 private:
  int status_code;            ///< The HTTP status code to return to the client (e.g. 502)
  std::string http_message;   ///< The short HTTP reason phrase (e.g. "Bad Gateway")
  std::string error_message;  ///< Detailed internal error description

 public:
  /**
   * @brief Constructs a new ProxyException.
   * @param status_code The HTTP status code.
   * @param http_message The HTTP status message.
   * @param error_message Detailed message about the error.
   */
  explicit ProxyException(int status_code, const std::string& http_message, const std::string& error_message);

  /**
   * @brief Returns the detailed internal error message.
   * @return const char* Pointer to the error message string.
   */
  const char* what() const noexcept override;

  /**
   * @brief Gets the associated HTTP status code.
   * @return int The HTTP status code.
   */
  int get_status_code() const;

  /**
   * @brief Gets the associated HTTP status message.
   * @return const std::string& The HTTP status message.
   */
  const std::string& get_http_message() const;

  /**
   * @brief Static helper to generate a formatted HTTP/1.1 error response page.
   *
   * Builds a raw HTTP response string including headers (Connection: close, Content-Type, Content-Length)
   * and a simple HTML body showing the status code and message.
   *
   * @param code The HTTP status code.
   * @param message The HTTP status message.
   * @return std::string The complete raw HTTP response.
   */
  static std::string generate_http_response(int code, const std::string& message);
};