# C++ Caching Proxy Server

A lightweight, multi-threaded Caching Proxy Server implemented in C++17. Designed to forward client HTTP/1.1 GET requests to a target origin server, cache responses using a thread-safe Sharded LRU Cache to minimize lock contention, and prevent the "thundering herd" problem through request coalescing.

## 🚀 Features

- **Multi-threaded Execution**: Employs a custom `ThreadPool` to handle multiple incoming client connections concurrently.
- **Low-Contention Sharded Cache**: Implements an LRU cache sharded across multiple locks to minimize thread synchronization overhead under heavy request volumes.
- **Thundering Herd Protection**: Uses a `RequestCoalescer` with C++ promise/future signals. Multiple concurrent requests for the same URL result in only a single upstream fetch; other clients await the result and receive a coalesced response directly from the cache.
- **Native HTTP Backend Fetching**: Built on top of `sockpp` for high-performance networking and `cpp-httplib` for reliable HTTP protocol parsing, including SSL support via OpenSSL.
- **Robust Integration Test Suite**: Includes a dedicated shell script testing concurrent request handling, cache hits/misses, and response validations.

---

## 🛠️ Tech Stack & Dependencies

*   **Language Standard**: C++17
*   **Build System**: CMake (v3.10+)
*   **Core Libraries**:
    *   [sockpp](https://github.com/fpagliughi/sockpp): Modern C++ wrapper for BSD sockets.
    *   [cpp-httplib](https://github.com/yhirose/cpp-httplib): Header-only C++ HTTP/HTTPS client.
    *   [OpenSSL](https://www.openssl.org/): For SSL/TLS support for secure backend servers.

---

## 📦 Prerequisites

Ensure you have CMake, OpenSSL headers, and a C++17-capable compiler installed. On Ubuntu/WSL:

```bash
sudo apt update
sudo apt install -y build-essential cmake libssl-dev
```

*Note: `sockpp` is automatically fetched and built as part of the CMake configuration step.*

---

## 🔧 Building the Project

Configure and build the binary using CMake:

```bash
# Configure the build directory
cmake -B build -S .

# Build the executable
cmake --build build
```

---

## 🏃 Running the Proxy

Start the proxy server by specifying a listening port and a target origin server:

```bash
./build/caching-proxy --port <port> --origin <origin_url>
```

### Example
To listen on port `3000` and proxy requests to `http://dummyjson.com`:

```bash
./build/caching-proxy --port 3000 --origin http://dummyjson.com
```

You can then curl the proxy server directly:
```bash
curl -I http://localhost:3000/products
```

### Custom Cache Markers
The proxy appends an `X-Cache` header to all client responses to indicate the cache status:
- `X-Cache: MISS`: The requested resource was fetched from the origin server.
- `X-Cache: HIT`: The resource was returned directly from the local LRU cache.

---

## 🧪 Testing

The repository contains an automated integration test script:

```bash
./test_proxy.sh
```

This script will:
1. Verify the project builds successfully.
2. Spin up the proxy server in the background.
3. Execute sequential GET requests to verify baseline Cache MISS and Cache HIT behavior.
4. Fire multiple concurrent requests to the same endpoint simultaneously to validate the **Request Coalescer** (ensures only 1 request goes upstream and the remaining concurrent requests are coalesced into Cache HITs).
5. Clean up background processes and output a colored summary.
