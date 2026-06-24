/**
 * @file Network.hpp
 * @brief Network listening, client dispatch, and backend origin communication interface.
 *
 * This file declares functions for starting the main TCP listening acceptor,
 * dispatching client requests, and handling upstream backend requests.
 */

#pragma once
#include <sockpp/tcp_socket.h>

#include <cstdint>
#include <string>

#include "cache/ShardedCache.hpp"
#include "concurrency/ThreadPool.hpp"
#include "error/ErrorHandler.hpp"
#include "network/RequestCoalescer.hpp"


/**
 * @brief Starts the primary TCP server socket acceptor loop.
 *
 * Listens on the specified port. Once a client connects, it encapsulates the
 * socket into a shared pointer and dispatches it to the thread pool for async execution.
 *
 * @param port The local port to bind the server to.
 * @param pool The ThreadPool to dispatch connection handler tasks to.
 * @param cache The sharded caching system.
 * @param origin The backend origin server base URL.
 * @param coalescer The request coalescing manager.
 */
void run_server(uint16_t port, ThreadPool& pool, ShardedCache& cache, const std::string& origin, RequestCoalescer& coalescer);

/**
 * @brief Handles the full lifecycle of a client HTTP proxy connection.
 *
 * Reads client requests, performs cache checks, utilizes coalescing to combine identical requests,
 * fetches missing resources from the origin backend, and writes the response back to the client.
 *
 * @param client The TCP socket connection with the client.
 * @param cache The sharded caching system.
 * @param origin The backend origin server base URL.
 * @param coalescer The request coalescing manager.
 */
void handle_client(sockpp::tcp_socket client, ShardedCache& cache, const std::string& origin, RequestCoalescer& coalescer);

/**
 * @brief Injects a custom HTTP header into an existing raw HTTP response.
 *
 * Appends the given header string (like "X-Cache: HIT\r\n") right before the HTTP header
 * termination sequence ("\r\n\r\n").
 *
 * @param response The mutable reference to the full HTTP response string.
 * @param header The header block string to inject.
 */
void inject_cache_header(std::string& response, const std::string& header);

/**
 * @brief Fetches a resource from the upstream origin backend using HTTP keep-alive.
 *
 * Leverages thread-local client connections to query the backend origin, sanitizes headers
 * that might break cache serialization, and constructs a clean HTTP/1.1 response string.
 *
 * @param origin The full origin server base URL.
 * @param path The path of the resource.
 * @return std::string The fully constructed HTTP/1.1 response from the backend.
 * @throws ProxyException on network issues or non-200 responses.
 */
std::string fetch_from_backend(const std::string& origin, const std::string& path, int* ttl);