#include "../include/logger/Logger.hpp"

#include <cstdlib>
#include <iostream>

Logger::Logger() { printer = std::thread(&Logger::process_queue, this); }

Logger::~Logger() {
  log_queue.stop();

  if (printer.joinable()) {
    printer.join();
  }

  if (log_file.is_open()) {
    log_file.close();
  }
}

Logger& Logger::get_instance() {
  static Logger instance;
  return instance;
}

void Logger::init(const std::string& f_name) {
  if (log_file.is_open()) {
    return;
  }

  filename = f_name;
  log_file.open(filename, std::ios::app);
}

void Logger::log(const std::string& message, LoggerLevel loggerLevel) {
  LogEntry entry;
  entry.level = loggerLevel;
  entry.timestamp = std::time(nullptr);
  entry.message = message;

  log_queue.push(std::move(entry));
}

void Logger::process_queue() {
  while (true) {
    auto entry = log_queue.pop();

    if (!entry.has_value()) {
      return;
    }

    std::string curr_time = get_timestamp(entry->timestamp);
    std::string level = get_level(entry->level);

    std::string fullMessage = "[Time : " + curr_time + "] [" + level + "] : " + entry->message;
    std::string consoleMessage = "[" + curr_time + "] [" + level + "] " + entry->message;

    std::cout << consoleMessage << '\n';
    if (log_file.is_open()) {
      log_file << fullMessage << '\n';
    }
  }
}

std::string Logger::get_timestamp(std::time_t raw_time) {
  struct tm timeinfo;
  localtime_r(&raw_time, &timeinfo);

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
