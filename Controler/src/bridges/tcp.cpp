
#include "tcp.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

namespace bridge {
namespace tcp {

  socket socket::open(const address& address)
  {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;

    /* Obtain address(es) matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;      /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;  /* TCP Connection */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;            /* Any protocol */

    s = getaddrinfo(address.host.c_str(), address.port.c_str(), &hints, &result);
    if (s != 0) {
      throw std::runtime_error("Failed get address infos : " 
        + std::string(gai_strerror(s)));
    }

    /* getaddrinfo() returns a list of address structures.
      Try each address until we successfully connect(2).
      If socket(2) (or connect(2)) fails, we (close the socket
      and) try the next address. */

    int socket_fd = 0;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
      socket_fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (socket_fd == -1)
      {
        perror("Invalid socket");
        continue;
      }

      if (::connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != -1)
        break;                  /* Success */

      perror("Can't connect");
      ::close(socket_fd);
    }
    freeaddrinfo(result); /* No longer needed */

    if (rp == NULL) {               /* No address succeeded */
      throw std::runtime_error("Failed connect to arduino");
    }

    if (-1 == fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL, 0) | O_NONBLOCK)) {
      int err = errno;
      throw std::runtime_error(strerror(err));
    }

    int keepcnt = 50;
    if (-1 == setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int))) {
      int err = errno;
      throw std::runtime_error(strerror(err));
    }

    int keepintl = 120;
    if (-1 == setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintl, sizeof(int))) {
      int err = errno;
      throw std::runtime_error(strerror(err));
    }

    return socket(socket_fd);
  }
  void socket::close()
  {
    if (_fd != 0) ::close(_fd);
  }

  void tcp_bridge::send_callback(const socket& sock, const config& cfg)
  {
    auto proxy = this->to_device();
    if (proxy._obj.empty())
      return;
    auto& pending = proxy._obj.front();
    const auto& payload = pending.second.payload;
    if (write(sock.fd(), payload.data(), payload.size()) != payload.size())
    {
      int err = errno;
      if (err == EAGAIN || err == EWOULDBLOCK)
        return;
      else
      {
        pending.first.set_value(packet::status::Failed);
        throw std::runtime_error(strerror(err));
      }
    }
    pending.first.set_value(packet::status::Sent);
    proxy._obj.pop();
  }

  void tcp_bridge::receive_callback(const socket& sock, const config& cfg)
  {
    char buffer[cfg.buffer_size];
    ssize_t nread;
    if (-1 == (nread = read(sock.fd(), buffer, cfg.buffer_size)))
    {
      int err = errno;
      if (err == EAGAIN || err == EWOULDBLOCK)
        return;
      else
        throw std::runtime_error(strerror(err));
    }
    buffer[nread] = '\0';
    if (nread != 0)
    {
      auto proxy = this->from_device();
      proxy._obj.emplace(std::string(buffer));
    }
  }
}
}