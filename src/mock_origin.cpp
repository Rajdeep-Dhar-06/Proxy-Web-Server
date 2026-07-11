// src/mock_origin.cpp
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "vendor/httplib.h"

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <port> <id> [fail_rate] [latency_ms]\n", argv[0]);
    return 1;
  }
  int port = std::atoi(argv[1]);
  std::string id = argv[2];
  double fail_rate = argc > 3 ? std::atof(argv[3]) : 0.0;
  int latency_ms = argc > 4 ? std::atoi(argv[4]) : 0;

  httplib::Server svr;

  svr.Get("/health", [](const httplib::Request&, httplib::Response& res) { res.set_content("ok", "text/plain"); });

  svr.Get(R"(/posts/(\d+))", [=](const httplib::Request& req, httplib::Response& res) {
    if (latency_ms) std::this_thread::sleep_for(std::chrono::milliseconds(latency_ms));
    if (fail_rate > 0 && (std::rand() / double(RAND_MAX)) < fail_rate) {
      res.status = 500;
      return;
    }
    res.set_header("X-Origin-Id", id);
    res.set_header("Cache-Control", "public, max-age=60");
    std::string post_id = req.matches[1];
    res.set_content("{\"id\":" + post_id + ",\"served_by\":\"" + id + "\"}", "application/json");
  });

  svr.Post(R"(/posts.*)", [=](const httplib::Request&, httplib::Response& res) {
    if (latency_ms) std::this_thread::sleep_for(std::chrono::milliseconds(latency_ms));
    res.set_header("X-Origin-Id", id);
    res.set_content("{\"created\":true,\"served_by\":\"" + id + "\"}", "application/json");
  });

  printf("mock-origin [%s] listening on :%d (fail_rate=%.2f, latency=%dms)\n", id.c_str(), port, fail_rate, latency_ms);
  svr.listen("0.0.0.0", port);
  return 0;
}