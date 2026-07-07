#include <sockpp/socket.h>

#include <iostream>
#include <regex>
#include <string>
#include <thread>

#include "config/config.hpp"
#include "logger/Logger.hpp"
#include "network/Network.hpp"

using namespace std;

bool is_valid_url(const string& url) {
  const regex url_regex(R"(^(https?)://[a-zA-Z0-9\-\.]+\.[a-zA-Z]{2,}(/.*)?$)");
  return regex_match(url, url_regex);
}

int main(int argc, char* argv[]) {
  Logger::get_instance().init();
  sockpp::initialize();

  if (argc != 5) {
    Logger::get_instance().log("Usage: caching-proxy --port <number> --origin <url>", LoggerLevel::ERROR);
    return 1;
  }

  vector<string> args(argv, argv + argc);

  int port = -1;
  string origin = "";

  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "--port") {
      if (i + 1 >= args.size()) {
        Logger::get_instance().log("--port requires a numeric value.", LoggerLevel::ERROR);
        return 1;
      }
      try {
        port = stoi(args[i + 1]);
        if (port < 1 || port > 65535) {
          Logger::get_instance().log("Port must be between 1 and 65535.", LoggerLevel::ERROR);
          return 1;
        }
        i++;
      } catch (const invalid_argument& e) {
        Logger::get_instance().log("--port requires a valid numeric value.", LoggerLevel::ERROR);
        return 1;
      } catch (const out_of_range& e) {
        Logger::get_instance().log("Port number is too large.", LoggerLevel::ERROR);
        return 1;
      }

    } else if (args[i] == "--origin") {
      if (i + 1 >= args.size()) {
        Logger::get_instance().log("--origin requires a URL.", LoggerLevel::ERROR);
        return 1;
      }
      origin = args[i + 1];

      if (!is_valid_url(origin)) {
        Logger::get_instance().log("Invalid origin URL. Must start with http:// or https://", LoggerLevel::ERROR);
        return 1;
      }
      i++;

    } else {
      Logger::get_instance().log("Unknown argument '" + args[i] + "'", LoggerLevel::ERROR);
      return 1;
    }
  }

  if (port != -1) {
    config.PORT = port;
  }
  if (!origin.empty()) {
    config.ORIGIN = origin;
  }

  if (config.ORIGIN.empty()) {
    Logger::get_instance().log("Target origin URL is required. Specify via --origin <url> or ORIGIN environment variable.",
                               LoggerLevel::ERROR);
    return 1;
  }

  try {
    std::cout << "========================================\n";
    std::cout << "Starting Proxy Server\n";
    std::cout << "Listening on Port:     " << config.PORT << "\n";
    std::cout << "Target Origin:         " << config.ORIGIN << "\n";
    std::cout << "Hardware threads:      " << std::thread::hardware_concurrency() << "\n";
    std::cout << "Worker pool size:      " << config.THREAD_COUNT << "\n";
    std::cout << "========================================\n";

    std::cout << "LRU cache online (" << config.CACHE_CAPACITY << " items / " << config.SHARDS_COUNT << " shards)\n";
    std::cout << "Thread pool initialized with " << config.THREAD_COUNT << " workers\n";

    // Start server
    ProxyServer server(config.PORT);
    server.run_server();
  } catch (const std::exception& e) {
    Logger::get_instance().log("Proxy crashed: " + std::string(e.what()), LoggerLevel::ERROR);
    return 1;
  }
  return 0;
}