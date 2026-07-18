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
    src/main.cpp
    config/config.cpp
    src/logger/Logger.cpp
    src/concurrency/ThreadPool.cpp
    src/cache/LRUCache.cpp
    src/cache/ShardedCache.cpp
    src/network/ProxyServer.cpp
    src/network/RequestCoalescer.cpp
    src/error/ErrorHandler.cpp
    src/vendor/picohttpparser.c
    src/middleware/ParseHandler.cpp
    src/middleware/CacheHandler.cpp
    src/middleware/UpstreamHandler.cpp
    src/utils/http_utils.cpp
    src/network/SocketConnection.cpp
)

# Specify the include directories
target_include_directories(caching-proxy PRIVATE include src .)

# Link threads library and sockpp
find_package(Threads REQUIRED)
target_link_libraries(caching-proxy PRIVATE Threads::Threads sockpp)
find_package(OpenSSL REQUIRED)
target_link_libraries(caching-proxy PRIVATE Threads::Threads sockpp OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(caching-proxy PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
if(ENABLE_TSAN)
    target_compile_options(caching-proxy PRIVATE -fsanitize=thread -g -O1)
    target_link_options(caching-proxy PRIVATE -fsanitize=thread)
endif()

enable_testing()
add_subdirectory(tests)


