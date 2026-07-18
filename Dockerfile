# Build stage
FROM debian:bookworm-slim AS builder

# Install build dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        libssl-dev \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy CMakeLists.txt to configure and build dependencies first
COPY CMakeLists.txt .

# Create dummy source files so CMake doesn't complain during configuration
RUN mkdir -p src config src/logger src/concurrency src/cache src/network src/error src/vendor src/middleware src/utils tests && \
    touch tests/CMakeLists.txt && \
    touch src/main.cpp config/config.cpp src/logger/Logger.cpp src/concurrency/ThreadPool.cpp \
          src/cache/LRUCache.cpp src/cache/ShardedCache.cpp src/network/ProxyServer.cpp \
          src/network/SocketConnection.cpp src/network/RequestCoalescer.cpp src/error/ErrorHandler.cpp \
          src/vendor/picohttpparser.c src/middleware/ParseHandler.cpp src/middleware/CacheHandler.cpp \
          src/middleware/UpstreamHandler.cpp src/utils/http_utils.cpp

# Configure and compile ONLY the external sockpp dependency
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make sockpp

# Copy the source code
COPY . .

# Build the main application
RUN cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# Runtime stage
FROM debian:bookworm-slim

# Install runtime dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libssl3 \
        ca-certificates \
        tzdata && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled binary and shared library from the builder stage
COPY --from=builder /app/build/caching-proxy /app/caching-proxy
COPY --from=builder /app/build/_deps/sockpp-build/libsockpp.so* /usr/local/lib/

# Update linker cache
RUN ldconfig

# Expose the default port
EXPOSE 8080

# Environment variables to configure the proxy
# e.g., PORT, ORIGIN, TTL
# ENV PORT=8080
# ENV ORIGIN=http://example.com

# Run the application
ENTRYPOINT ["/app/caching-proxy"]
