#pragma once
#include <cstddef>

enum class IoResult { Ok, Closed, Error };

class IConnection {
 public:
  // Constructor and destructor
  virtual ~IConnection() = default;

  // Connection Operations
  virtual IoResult read(char* buf, size_t len, size_t& bytes_read) = 0;
  virtual IoResult write(const char* buf, size_t len) = 0;
  virtual void close() = 0;
};
