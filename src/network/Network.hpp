#pragma once
#include "RequestCoaleser.hpp"
#include "../cache/ShardedCache.hpp"
#include "../concurrency/ThreadPool.hpp"
#include "../error/ErrorHandler.hpp"
#include <cstdint>
#include <sockpp/tcp_socket.h>
#include <string>

using namespace std;

void run_server(uint16_t port, ThreadPool &pool, ShardedCache &cache, const std::string &origin,  RequestCoalescer& coalescer);
void handle_client(sockpp::tcp_socket client, ShardedCache &cache, const std::string &origin, RequestCoalescer& coalescer);
void inject_cache_header(std::string& response, const std::string& header);
std::string fetch_from_backend(const std::string& host, int port, const std::string& path);