# C++ Caching Proxy Server

A high-performance, multi-threaded Caching Proxy Server implemented in C++17. This server intercepts client HTTP/1.1 requests, checks a thread-safe Sharded LRU Cache to serve cached responses immediately, coalesces concurrent requests to the same target resource to mitigate backend load, and forwards cache misses to the destination origin server.

---

## Technical Architecture

The server is built with modern systems programming patterns to ensure high throughput, safety, and correctness under concurrent workloads:

1. **Multi-threaded Worker Pool (ThreadPool)**: Rather than spawning a thread per TCP connection, the server pre-allocates a fixed-size worker thread pool. Connections accepted by the main thread are enqueued onto a task queue, minimizing context-switching overhead and bounding resource consumption.
2. **Lock-Contention Mitigation (Sharded Cache)**: A single global cache requires a single mutex, making it a severe concurrency bottleneck. This implementation shards the LRU cache into 16 independent partition buckets. Requests are routed to a specific shard using the hash of their URL key, allowing concurrent threads to read and write without lock contention.
3. **Thundering Herd Protection (Request Coalescing)**: To prevent cache stampede when multiple clients request an uncached resource at the same time, the server implements request coalescing. Using C++17 `std::promise` and `std::shared_future`, only the first request (the owner) fetches from the backend. Concurrent waiters block on the shared future, automatically resolving and serving the response from the cache once the owner finishes.
4. **Incremental Stream Parsing (picohttpparser)**: TCP is a stream-oriented protocol where HTTP headers may arrive fragmented across network packets. The server utilizes the fast C-based `picohttpparser` in an incremental socket-reading loop to correctly handle partial packet header fragmentation.
5. **OpenSSL/HTTPS Backend Support**: The network client natively supports fetching from secure HTTPS targets as well as standard HTTP backends by conditionally instantiating SSL-enabled clients.

---

## Directory Layout

```text
caching-proxy/
├── CMakeLists.txt              # CMake build configuration
├── README.md                   # Project documentation
├── test_proxy.sh               # Integration test script
├── include/                    # Header declarations
│   ├── cache/
│   │   ├── LRUCache.hpp
│   │   └── ShardedCache.hpp
│   ├── concurrency/
│   │   └── ThreadPool.hpp
│   ├── error/
│   │   └── ErrorHandler.hpp
│   ├── network/
│   │   ├── HTTPParser.hpp
│   │   ├── Network.hpp
│   │   └── RequestCoalescer.hpp
│   └── vendor/
│       ├── httplib.h
│       └── picohttpparser.h
└── src/                        # Source implementations
    ├── main.cpp
    ├── cache/
    │   ├── LRUCache.cpp
    │   └── ShardedCache.cpp
    ├── concurrency/
    │   └── ThreadPool.cpp
    ├── error/
    │   └── ErrorHandler.cpp
    ├── network/
    │   ├── HTTPParser.cpp
    │   ├── Network.cpp
    │   └── RequestCoalescer.cpp
    └── vendor/
        └── picohttpparser.c
```

---

## Tech Stack & Dependencies

*   **Language Standard**: C++17
*   **Build System**: CMake (v3.10+)
*   **Core Libraries**:
    *   [sockpp](https://github.com/fpagliughi/sockpp): Modern C++ socket wrapper.
    *   [cpp-httplib](https://github.com/yhirose/cpp-httplib): C++ HTTP/HTTPS client library.
    *   [OpenSSL](https://www.openssl.org/): For SSL/TLS secure backend connections.
    *   [picohttpparser](https://github.com/h2o/picohttpparser): High-performance HTTP parser.

---

## Prerequisites

To compile the codebase, ensure you have CMake, a C++17 compiler (GCC or Clang), and OpenSSL libraries installed on your machine.

For Ubuntu/Debian/WSL:
```bash
sudo apt update
sudo apt install -y build-essential cmake libssl-dev
```
---

## Building the Project

Configure and build the executable using CMake:

```bash
# Configure the build directory
cmake -B build -S .

# Build the executable target
cmake --build build
```

---

## Running the Proxy

Start the proxy server by specifying a local port and the target origin backend URL:

```bash
./build/caching-proxy --port <port> --origin <origin_url>
```

### Example
To run the proxy server locally on port 3000 and target `http://dummyjson.com`:

```bash
./build/caching-proxy --port 3000 --origin http://dummyjson.com
```

In another terminal, you can perform requests to the proxy:
```bash
curl -I http://localhost:3000/products
```

### Cache Response Markers
The server injects an `X-Cache` header in its responses to indicate cache state:
*   `X-Cache: MISS`: The resource was fetched from the upstream origin.
*   `X-Cache: HIT`: The resource was returned directly from the sharded LRU cache.

---

## Testing

An automated shell script is provided to verify correctness, including concurrent request handling and coalescing:

```bash
./test_proxy.sh
```

The test script:
1. Validates that the project compiles successfully.
2. Starts the proxy server in the background targeting a mockable origin.
3. Tests cache miss and cache hit sequencing.
4. Spawns 5 concurrent requests simultaneously to verify request coalescing (confirming only 1 request hits the backend while the remaining 4 are cleanly served as coalesced hits).
5. Stops background processes and prints a test result report.
