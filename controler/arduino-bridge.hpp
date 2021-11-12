#pragma once

#include <string>
#include <vector>
#include <optional>

class ArduinoBridge {

  int socket_fd;

public:
  ArduinoBridge(const char* host, const char* port);
  ~ArduinoBridge();

  void send(const std::vector<uint8_t>& packet);
  std::string receive();
};