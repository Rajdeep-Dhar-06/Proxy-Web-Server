#include <sockpp/socket.h>

#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include "network/Network.hpp"
using namespace std;
constexpr int CACHE_CAPACITY = 1000;
constexpr int CACHE_SHARDS = 16;

unsigned compute_thread_count() {
  unsigned hc = std::thread::hardware_concurrency();
  if (hc == 0) hc = 4;
  return hc * 2;
}

bool isValidURL(const string& url) {
  const regex url_regex(R"(^(http)://[a-zA-Z0-9\-\.]+\.[a-zA-Z]{2,}(/.*)?$)");
  return regex_match(url, url_regex);
}

int main(int argc, char* argv[]) {
  sockpp::initialize();

  if (argc != 5) {
    cerr << "Usage: caching-proxy --port <number> --origin <url>\n";
    return 1;
  }

  vector<string> args(argv, argv + argc);

  int port = -1;
  string origin = "";

  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "--port") {
      if (i + 1 >= args.size()) {
        cerr << "Error: --port requires a numeric value.\n";
        return 1;
      }
      try {
        port = stoi(args[i + 1]);
        if (port < 1 || port > 65535) {
          cerr << "Error: Port must be between 1 and 65535.\n";
          return 1;
        }
        i++;
      } catch (const invalid_argument& e) {
        cerr << "Error: --port requires a valid numeric value.\n";
        return 1;
      } catch (const out_of_range& e) {
        cerr << "Error: Port number is too large.\n";
        return 1;
      }

    } else if (args[i] == "--origin") {
      if (i + 1 >= args.size()) {
        cerr << "Error: --origin requires a URL.\n";
        return 1;
      }
      origin = args[i + 1];

      if (!isValidURL(origin)) {
        cerr << "Error: Invalid origin URL. Must start with http://\n";
        return 1;
      }
      i++;

    } else {
      cerr << "Error: Unknown argument '" << args[i] << "'\n";
      return 1;
    }
  }

  if (port == -1 || origin.empty()) {
    cerr << "Error: Both --port and --origin are required to start the proxy.\n";
    return 1;
  }

  unsigned num_threads = compute_thread_count();

  try {
    cout << "========================================\n";
    cout << "Starting Proxy Server \n";
    cout << "Listening on Port:     " << port << "\n";
    cout << "Target Origin:         " << origin << "\n";
    cout << "Hardware threads:      " << std::thread::hardware_concurrency() << "\n";
    cout << "Worker pool size:      " << num_threads << "\n";
    cout << "========================================\n";

    ShardedCache cache(CACHE_CAPACITY, CACHE_SHARDS);
    cout << "[SYSTEM] LRU cache online (" << CACHE_CAPACITY << " items / " << CACHE_SHARDS << " shards)\n";

    ThreadPool pool(num_threads);
    RequestCoalescer rc;
    cout << "[SYSTEM] Thread pool initialized with " << num_threads << " workers\n";

    run_server(port, pool, cache, origin, rc); 
  } catch (const std::exception& e) {
    cerr << "\n[FATAL ERROR] Proxy crashed: " << e.what() << "\n";
    return 1;
  }
  return 0;
}