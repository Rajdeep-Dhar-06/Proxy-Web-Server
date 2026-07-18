cmake_minimum_required(VERSION 3.10)
project(caching-proxy-tests CXX)

# Fetch GTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(run_tests
    cache/LRUCacheTest.cpp
    cache/ShardedCacheTest.cpp
    parser/HttpParserTest.cpp
    coalescer/RequestCoalescerTest.cpp
    
    # Implementation files
    ${CMAKE_SOURCE_DIR}/config/config.cpp
    ${CMAKE_SOURCE_DIR}/src/logger/Logger.cpp
    ${CMAKE_SOURCE_DIR}/src/cache/LRUCache.cpp
    ${CMAKE_SOURCE_DIR}/src/cache/ShardedCache.cpp
    ${CMAKE_SOURCE_DIR}/src/network/RequestCoalescer.cpp
    ${CMAKE_SOURCE_DIR}/src/error/ErrorHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/vendor/picohttpparser.c
    ${CMAKE_SOURCE_DIR}/src/utils/http_utils.cpp
)

target_include_directories(run_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/src 
    ${CMAKE_SOURCE_DIR}
)

# OpenSSL dependency
find_package(OpenSSL REQUIRED)

target_link_libraries(run_tests PRIVATE 
    gtest_main 
    gtest 
    Threads::Threads 
    sockpp 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

target_compile_definitions(run_tests PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

add_test(NAME proxy_unit_tests COMMAND run_tests)
