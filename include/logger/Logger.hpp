#pragma once

#include <ctime>
#include <fstream>
#include <string>
#include <thread>

#include "../structures/ThreadSafeQueue.hpp"

enum class LoggerLevel { INFO, WARNING, DEBUG, ERROR };

struct LogEntry {
  LoggerLevel level;
  std::time_t timestamp;
  std::string message;
};

class Logger {
 public:
  // Singleton access point
  static Logger& get_instance();

  // Add log file
  void init(const std::string& filename);

  // Logging method
  void log(const std::string& message, LoggerLevel loggerlevel = LoggerLevel::INFO);

 private:
  // Private constructor and destructor
  Logger();
  ~Logger();

  // Delete copy operations
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  // Helpers
  std::string get_timestamp(std::time_t raw_time);
  std::string get_level(LoggerLevel level);

  void process_queue();

  // Members
  std::string filename;
  std::ofstream log_file;

  // Concurrency primitives
  ThreadSafeQueue<LogEntry> log_queue;
  std::thread printer;
};