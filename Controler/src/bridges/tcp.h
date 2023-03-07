#pragma once

#include "bridge.h"

#include <thread>
#include <future>

namespace bridge {
namespace tcp {

class socket {
public :

  /// @brief 
  /// @param address 
  /// @return the successfully created socket, throw on failure 
  static socket open(const address& address);

  explicit constexpr operator bool() const noexcept
  { return _fd != 0; }

  socket(const socket&) = delete;
  socket& operator= (const socket&) = delete;

  socket(socket&& s) : _fd{s._fd} { s._fd = 0; }
  socket& operator= (socket&& s){ this->close(); _fd = s._fd; s._fd = 0; return *this; };

  constexpr int fd() const noexcept { return _fd; }

  constexpr socket() : socket(0) {}
  ~socket() { close(); }

private :
  constexpr socket(int fd) : _fd{fd} {}
  void close();

  int _fd;
};

struct config {
  std::chrono::milliseconds send_slewrate = std::chrono::milliseconds(10);
  size_t buffer_size = 1024;
};

class tcp_bridge : public async_bridge {
public:

  using config_proxy = utils::locked<config>::proxy;

  tcp_bridge(const address& address, const config& cfg = config())
  : async_bridge(&tcp_bridge::threadfn, this, socket::open(address)),
  _cfg(std::mutex(), cfg)
  {}

  const config& get_config() const noexcept { return _cfg._obj; }
  config_proxy set_config() noexcept { return _cfg.lock(); }

private:

  static void threadfn(std::stop_token stoken, tcp_bridge* bridge, socket sock)
  {
    while (!stoken.stop_requested() && bridge->alive())
    {
      auto cfg = bridge->get_config();
      bridge->send_callback(sock, cfg);
      bridge->receive_callback(sock, cfg);
      std::this_thread::sleep_for(std::chrono::milliseconds(cfg.send_slewrate));
    }
  }

  void send_callback(const socket& sock, const config& cfg);
  void receive_callback(const socket& sock, const config& cfg);

  
  utils::locked<config> _cfg;
};

}
}