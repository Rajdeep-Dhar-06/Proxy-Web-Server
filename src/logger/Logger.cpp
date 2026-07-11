#include "../include/logger/Logger.hpp"

#include <fstream>
#include <iostream>
#include <mutex>

static std::mutex mtx;

Logger::Logger() {}
Logger::~Logger() {
  if (log_file.is_open()) {
    log_file.close();
  }
}

Logger& Logger::get_instance() {
  // Thread safe creation, local static initialisation
  static Logger instance;
  return instance;
}

void Logger::init(const std::string& f_name) {
  std::lock_guard<std::mutex> lock(mtx);

  if (log_file.is_open()) {
    return;
  }

  filename = std::move(f_name);
  log_file.open(filename, std::ios::app);
}

void Logger::log(const std::string& message, LoggerLevel loggerLevel) {
  std::string curr_time = get_timestamp();
  std::string level = get_level(loggerLevel);
  std::string fullMessage = "[Time : " + curr_time + "] [" + level + "] : " + message;
  std::string consoleMessage = "[" + curr_time + "] [" + level + "] " + message;

  // std::lock_guard<std::mutex> lock(mtx);
  // std::cout << consoleMessage << '\n';
  // if (log_file.is_open()) {
  //   log_file << fullMessage << '\n';
  // }
}

std::string Logger::get_timestamp() {
  std::time_t now = std::time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  char buffer[20];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return std::string(buffer);
}

std::string Logger::get_level(LoggerLevel level) {
  switch (level) {
    case LoggerLevel::INFO:
      return "INFO";
    case LoggerLevel::WARNING:
      return "WARNING";
    case LoggerLevel::ERROR:
      return "ERROR";
    case LoggerLevel::DEBUG:
      return "DEBUG";
    default:
      return "UNKNOWN";
  }
}