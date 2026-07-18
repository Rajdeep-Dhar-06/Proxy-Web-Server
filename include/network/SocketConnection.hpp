#pragma once
#include <sockpp/tcp_socket.h>

#include "IConnection.hpp"

class SocketConnection : public IConnection {
 public:
  // Constructor and destructor
  explicit SocketConnection(sockpp::tcp_socket sock) : socket(std::move(sock)) {}

  // Socket Operations
  IoResult read(char* buf, size_t len, size_t& bytes_read) override;
  IoResult write(const char* buf, size_t len) override;
  void close() override;

 private:
  // Members
  sockpp::tcp_socket socket;
};
