#include "tcp.h"

#ifdef __unix__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>

#endif

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdexcept>

namespace transport {
namespace tcp {

packet_status socket::vsend(const packet_payload& p)
{
  if (write(_fd, p.data(), p.size()) != p.size())
  {
    int err = errno;
    if (err == EAGAIN || err == EWOULDBLOCK)
      return packet_status::Failed;
    else
      throw std::runtime_error(strerror(err));
  }
  else
    return packet_status::Sent;
}

opt_reply socket::vreceive()
{
  std::string buffer;
  buffer.resize(_cfg.buffer_size);

  ssize_t nread;
  if (-1 == (nread = read(_fd, buffer.data(), _cfg.buffer_size)))
  {
    int err = errno;
    if (err == EAGAIN || err == EWOULDBLOCK)
      return std::nullopt;
    else
      throw std::runtime_error(strerror(err));
  }
  buffer.resize(nread);

  return std::make_optional<reply>(std::move(buffer));
}

#ifdef __unix__

/// @brief Try to open a linux tcp non blocking connection to given host
/// @param addr address of the server to connect to
/// @return opened socket's file descriptor on success, throw on failure
int socket::open(const socket_config& cfg)
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

  auto addr = std::get<address>(cfg);

  s = getaddrinfo(addr.host.c_str(), addr.port.c_str(), &hints, &result);
  if (s != 0) {
    throw std::runtime_error("tcp open : getaddrinfo : " 
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

  /// Set the socket in Non-Blocking mode
  if (-1 == fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL, 0) | O_NONBLOCK)) {
    int err = errno;
    throw std::runtime_error(strerror(err));
  }

  auto kcfg = std::get<keepalive_config>(cfg);

  /// Configure TCP Keep Alive to sent 50 messages before killing the connection
  int keepcnt = kcfg.counter;
  if (-1 == setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int))) {
    int err = errno;
    throw std::runtime_error(strerror(err));
  }

  /// Configure TCP Keep Alive to probe the connection every 120 seconds
  int keepintl = kcfg.interval;
  if (-1 == setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintl, sizeof(int))) {
    int err = errno;
    throw std::runtime_error(strerror(err));
  }

  return socket_fd;
}

/// @brief Close the holded file descriptor if exists, throw on failure
void socket::close(int fd)
{
  if (fd != 0)
    ::close(fd);
}

#else // __unix __
#ifdef __WIN32__

int socket::open(const socket_config&) { return 0; }
void socket::close(int fd) {}

#endif // __WIN32__
#endif

}
}