#include "network/SocketConnection.hpp"

IoResult SocketConnection::read(char* buf, size_t len, size_t& bytes_read) {
  auto n = socket.read(buf, len); // read from connection socket
  if (n > 0) {
    bytes_read = static_cast<size_t>(n);
    return IoResult::Ok;
  }
  if (n == 0) {
    return IoResult::Closed; // peer closed the connection
  }
  return IoResult::Error; // socket error encountered
}

IoResult SocketConnection::write(const char* buf, size_t len) {
  auto n = socket.write_n(buf, len); // write to connection socket
  if (n >= 0 && len == static_cast<size_t>(n)) {
    return IoResult::Ok; // all bytes written successfully
  }
  return IoResult::Error;
}

void SocketConnection::close() { socket.close(); } // shut down connection socket
