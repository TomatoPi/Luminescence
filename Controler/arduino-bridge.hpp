#pragma once

#include "thread-queue.hpp"

#include <string>
#include <vector>
#include <thread>
#include <utility>
#include <optional>
#include <atomic>

class ArduinoBridge {

  using packet_t = std::vector<uint8_t>;
  using pending_obj_t = std::pair<size_t, packet_t>;

  ThreadSafeQueue<pending_obj_t> sending_queue;
  std::thread sending_thread;

  int socket_fd;
  const char* host, *port;

  std::atomic_flag is_active;
  std::atomic_flag shutdown;

  bool connect();

  static void callback(ArduinoBridge* bridge);

public:
  ArduinoBridge(const char* host, const char* port);
  ~ArduinoBridge();

  void send(size_t addr, const packet_t& packet);
  std::optional<std::string> receive();

  void kill();
};
