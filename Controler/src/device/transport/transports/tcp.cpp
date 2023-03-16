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

#ifdef __unix__

/// @brief Try to open a linux tcp non blocking connection to given host
/// @param addr address of the server to connect to
/// @return opened socket's file descriptor on success, throw on failure
int open_socket(const std::tuple<address, keepalive_config, fd::read_buffer_size>& sig)
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

  auto addr = std::get<address>(sig);
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
    throw std::runtime_error("No address succeeded");
  }

  /// Set the socket in Non-Blocking mode
  if (-1 == fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL, 0) | O_NONBLOCK)) {
    int err = errno;
    throw std::runtime_error(strerror(err));
  }

  /* *** KEEP ALIVE *** */

  /// Configure TCP Keep Alive to sent 50 messages before killing the connection
  auto kcfg = std::get<keepalive_config>(sig);
  int keepcnt = kcfg.counter;
  if (-1 == setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(int))) {
    int err = errno;
    throw std::runtime_error(std::string("tcp open : stdsockopt(TCP_KEEPCNT) : " + std::string(strerror(err))));
  }

  /// Configure TCP Keep Alive to probe the connection every 120 seconds
  int keepintl = kcfg.interval;
  if (-1 == setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintl, sizeof(int))) {
    int err = errno;
    throw std::runtime_error(std::string("tcp open : stdsockopt(TCP_KEEPINTVL) : " + std::string(strerror(err))));
  }

  return socket_fd;
}

/// @brief Close the holded file descriptor if exists, throw on failure
void close_socket(int fd)
{
  if (fd != 0)
    ::close(fd);
}

#endif // __unix __
#ifdef __WIN32__

int open_socket(const std::tuple<address, keepalive_config, fd::read_buffer_size>& sig)
{ return 0; }

void close_socket(int fd)
{}

#endif // __WIN32__

}
}