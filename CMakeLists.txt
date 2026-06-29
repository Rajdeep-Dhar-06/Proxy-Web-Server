cmake_minimum_required(VERSION 3.10)
project(caching-proxy C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    sockpp
    GIT_REPOSITORY https://github.com/fpagliughi/sockpp.git
    GIT_TAG        v1.0.0
)

FetchContent_MakeAvailable(sockpp)

# Disable sockpp's own examples and tests to keep build quick
set(SOCKPP_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(SOCKPP_BUILD_TESTS OFF CACHE INTERNAL "")

add_executable(caching-proxy
    utils.cpp
    src/main.cpp
    src/concurrency/ThreadPool.cpp
    src/cache/LRUCache.cpp
    src/cache/ShardedCache.cpp
    src/network/Network.cpp
    src/network/HTTPParser.cpp
    src/network/RequestCoalescer.cpp
    src/error/ErrorHandler.cpp
    src/vendor/picohttpparser.c
    src/middleware/ParseHandler.cpp
    src/middleware/CacheHandler.cpp
    src/middleware/UpstreamHandler.cpp
)

# Specify the include directories
target_include_directories(caching-proxy PRIVATE include src)

# Link threads library and sockpp
find_package(Threads REQUIRED)
target_link_libraries(caching-proxy PRIVATE Threads::Threads sockpp)
find_package(OpenSSL REQUIRED)
target_link_libraries(caching-proxy PRIVATE Threads::Threads sockpp OpenSSL::SSL OpenSSL::Crypto)

# Enable testing framework
enable_testing()

# Build the cache test executable
add_executable(test_cache
    tests/test_cache.cpp
    src/cache/LRUCache.cpp
    src/cache/ShardedCache.cpp
)
target_include_directories(test_cache PRIVATE include)
target_link_libraries(test_cache PRIVATE Threads::Threads)

# Add cache_test to CTest
add_test(NAME cache_test COMMAND test_cache)

