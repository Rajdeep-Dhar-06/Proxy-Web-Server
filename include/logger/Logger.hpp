#pragma once
#include <fstream>
#include <string>
enum class LoggerLevel { INFO, WARNING, DEBUG, ERROR };

class Logger {
 public:
  // Singleton access point
  static Logger& get_instance();

  // Add log file
  void init(const std::string& filename = "ProxyServerLog.log");

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
  std::string get_timestamp();
  std::string get_level(LoggerLevel level);
  int get_severity(LoggerLevel level);

  // Members
  std::string filename;
  std::ofstream log_file;
  LoggerLevel min_level = LoggerLevel::INFO;
};