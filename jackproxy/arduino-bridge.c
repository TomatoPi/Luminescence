#define _GNU_SOURCE
#include "arduino-serial-lib.h"
#include <termios.h>

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

volatile int is_running = 1;

void sighandler(int sig)
{
	switch(sig)
	{
	case SIGTERM:
	case SIGINT:
		is_running = 0;
		break;
	}
}

int main(int argc, char* const argv[])
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	ssize_t nread;

  if (argc != 3)
  {
    fprintf(stderr, "Usage : %s <port> <serial-port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	hints.ai_protocol = 0;          /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	s = getaddrinfo(NULL, argv[1], &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}
	
	/* getaddrinfo() returns a list of address structures.
	Try each address until we successfully bind(2).
	If socket(2) (or bind(2)) fails, we (close the socket
	and) try the next address. */

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype,
		       rp->ai_protocol);
		if (sfd == -1)
		   continue;

		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
		   break; /* Success */

		close(sfd);
	}

	if (rp == NULL)
	{	/* No address succeeded */
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result); /* No longer needed */

  int serialfd = serialport_init(argv[2], B115200);
  if (serialfd < 0)
  {
    fprintf(stderr, "Can't open serial port\n");
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "Connected to arduino OK\n");

	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);

  pid_t cpid = fork();
  if (-1 == cpid)
  {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  char* buffer[512];

  if (0 == cpid) // Child : Arduino -> stdout
  {
    while (is_running)
    {
      ssize_t nread = read(serialfd, buffer, 1);
      if (-1 == nread)
      {
        if (EAGAIN == errno || EWOULDBLOCK == errno)
        {
          usleep(100);
          continue;
        }
        else
        {
          perror("read from arduino");
          exit(EXIT_FAILURE);
        }
      }
      if (fwrite(buffer, 1, nread, stdout) != nread)
      {
        perror("write to stdout");
        exit(EXIT_FAILURE);
      }
      fflush(stdout);
      usleep(100);
    }
  }
  else // 0 != cpid : Parent : stdin -> arduino
  {
    while (is_running)
    {
      peer_addr_len = sizeof(struct sockaddr_storage);
      nread = recvfrom(sfd, buffer, 512, 0,
            (struct sockaddr *) &peer_addr, &peer_addr_len);
      if (nread == -1)
        continue;               /* Ignore failed request */

      char host[NI_MAXHOST], service[NI_MAXSERV];

      s = getnameinfo((struct sockaddr *) &peer_addr,
              peer_addr_len, host, NI_MAXHOST,
              service, NI_MAXSERV, NI_NUMERICSERV);
      if (s == 0)
      {
        if (write(serialfd, buffer, nread) != nread)
        {
          perror("write to arduino");
          exit(EXIT_FAILURE);
        }
        serialport_flush(serialfd);
        fprintf(stderr, "Wrote %zd bytes to arduino\n", nread);
        usleep(100);
      }
      else
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

    }
    kill(cpid, SIGTERM);
  }

  return 0;
}