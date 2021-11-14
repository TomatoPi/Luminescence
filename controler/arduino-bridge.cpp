#include "arduino-bridge.hpp"
#include "thread-queue.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdexcept>
#include <unordered_map>

ArduinoBridge::ArduinoBridge(const char* host, const char* port) :
  host(host), port(port)
{
  sending_thread = std::thread([](ArduinoBridge* bridge){
  while (1)
  {
  try {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;

    /* Obtain address(es) matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    s = getaddrinfo(bridge->host, bridge->port, &hints, &result);
    if (s != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
      exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
      Try each address until we successfully connect(2).
      If socket(2) (or connect(2)) fails, we (close the socket
      and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
      bridge->socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (bridge->socket_fd == -1)
      {
        perror("Invalid socket");
	continue;
      }

      if (connect(bridge->socket_fd, rp->ai_addr, rp->ai_addrlen) != -1)
        break;                  /* Success */

      perror("Can't connect");
      close(bridge->socket_fd);
    }

    if (rp == NULL) {               /* No address succeeded */
      throw std::runtime_error("Failed connect to arduino");
    }

    freeaddrinfo(result); /* No longer needed */

    std::unordered_map<size_t, packet_t> pending_objects;
    while (1)
    {
      std::optional<pending_obj_t> optobj;
      while (std::nullopt != (optobj = bridge->sending_queue.pop()))
      {
        auto [addr, obj] = optobj.value();
        pending_objects.insert_or_assign(addr, obj);
      }
      if (!pending_objects.empty())
      {
        auto itr = pending_objects.begin();
        auto [_, packet] = *itr;
        if (write(bridge->socket_fd, packet.data(), packet.size()) != packet.size())
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
          {
            usleep(100);
            continue;
          }
          else
          {
            perror("Partial / failed write\n");
            throw std::runtime_error("Err");
          }
        }
        // Sleep after object send to avoid network congestion
        pending_objects.erase(itr);
        usleep(1000);
      }
      else
      	usleep(100);
    }
  }
  catch (const std::runtime_error& err) {
    fprintf(stderr, "Connection terminated ... trying to reconnect\n");
    usleep(1'000'000);
    continue;
  }
  } // while
  }, this);
}
ArduinoBridge::~ArduinoBridge()
{
  close(socket_fd);
}

void ArduinoBridge::send(size_t addr, const packet_t& packet)
{
  sending_queue.push(pending_obj_t(addr, packet));
}
std::string ArduinoBridge::receive()
{
  char buffer[512];
  ssize_t nread;
  while (-1 == (nread = read(socket_fd, buffer, 512)))
  {
    usleep(1'000'000);
    continue;
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
      usleep(100);
      continue;
    }
    else
    {
      perror("Failed read from network\n");
      throw std::runtime_error("Err");
    }
  }

  buffer[nread] = '\0';
  return std::string(buffer);
}
