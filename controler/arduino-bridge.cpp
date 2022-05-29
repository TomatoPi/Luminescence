#include "arduino-bridge.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdexcept>

ArduinoBridge::ArduinoBridge(const char* host, const char* port)
{
  struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s;
	
	/* Obtain address(es) matching host/port */

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

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
		socket_fd = socket(rp->ai_family, rp->ai_socktype,
			    rp->ai_protocol);
		if (socket_fd == -1)
		   continue;

		if (connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != -1)
		   break;                  /* Success */

		close(socket_fd);
	}

	if (rp == NULL) {               /* No address succeeded */
		throw std::runtime_error("Failed connect to arduino");
	}

	freeaddrinfo(result); /* No longer needed */

}
ArduinoBridge::~ArduinoBridge()
{
  close(socket_fd);
}

void ArduinoBridge::send(const std::vector<uint8_t>& packet)
{
  while (write(socket_fd, packet.data(), packet.size()) != packet.size())
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
}
std::string ArduinoBridge::receive()
{
  char buffer[512];
  ssize_t nread;
  while (-1 == (nread = read(socket_fd, buffer, 512)))
  {
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