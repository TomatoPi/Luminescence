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
#include <iostream>
#include <unordered_map>

ArduinoBridge::ArduinoBridge(const char* host, const char* port) :
  host{host}, port{port}, is_active{ATOMIC_FLAG_INIT}, shutdown{ATOMIC_FLAG_INIT}
{
  is_active.clear();
  shutdown.test_and_set();
  sending_thread = std::thread(callback, this);
}
ArduinoBridge::~ArduinoBridge()
{
  close(socket_fd);
}

void ArduinoBridge::callback(ArduinoBridge* bridge)
{
  std::cout << "Starting Arduino Bridge" << std::endl;
  while (bridge->shutdown.test_and_set())
  {
    try {

      std::cout << "Wait for connection to the driver ... " << '\n';
      
      if (!bridge->connect())
        throw std::runtime_error("Failed connect to arduino");

      std::cout << '\b' << "Connection accepted" << '\n';

      std::unordered_map<size_t, packet_t> pending_objects;
      while (bridge->shutdown.test_and_set())
      {
        // std::cout << "Connection Loop Begin" << '\n';
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
        // std::cout << "Connection Loop end" << '\n';
      }
    }
    catch (const std::runtime_error& err) {
      std::cerr << err.what() << '\n';
      std::cerr << "Connection terminated ... trying to reconnect" << std::endl;
      usleep(1'000'000);
      close(bridge->socket_fd);
      bridge->is_active.clear();
      continue;
    }
    std::cout << "Connection closed" << std::endl;
  } // while
  std::cout << "Shuting down Arduino Bridge" << std::endl;
}

bool ArduinoBridge::connect()
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

  s = getaddrinfo(host, port, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  /* getaddrinfo() returns a list of address structures.
    Try each address until we successfully connect(2).
    If socket(2) (or connect(2)) fails, we (close the socket
    and) try the next address. */

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (socket_fd == -1)
    {
      perror("Invalid socket");
      continue;
    }

    if (::connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;                  /* Success */

    perror("Can't connect");
    close(socket_fd);
  }

  if (rp == NULL) {               /* No address succeeded */
    throw std::runtime_error("Failed connect to arduino");
  }

  freeaddrinfo(result); /* No longer needed */
  is_active.test_and_set();
  return true;
}

void ArduinoBridge::kill()
{
  shutdown.clear();
  is_active.clear();
}


void ArduinoBridge::send(size_t addr, const packet_t& packet)
{
  sending_queue.push(pending_obj_t(addr, packet));
}
std::optional<std::string> ArduinoBridge::receive()
{
  if (!is_active.test_and_set())
    return std::nullopt;

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
  return std::make_optional(std::string(buffer));
}
