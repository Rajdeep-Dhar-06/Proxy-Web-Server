#include <cstdint>
#include <string>

struct Config {
    uint16_t PORT        = 8080;
    std::string ORIGIN;
    int TTL              = 60; // seconds
    int SHARDS_COUNT     = 16;
    int THREAD_COUNT     = 24;
    int CACHE_CAPACITY   = 1024;
    int CONNECT_TIMEOUT  = 5; // seconds
    int READ_TIMEOUT     = 10;
    int WRITE_TIMEOUT    = 5;
    int MAX_HEADER_BYTES = 8192;

    static Config from_env();
};