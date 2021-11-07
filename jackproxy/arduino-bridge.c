#define _GNU_SOURCE
#include "arduino-serial-lib.h"
#include <termios.h>

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

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
  if (argc != 2)
  {
    fprintf(stderr, "Usage : %s <serial-port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int serialfd = serialport_init(argv[1], B115200);
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
      ssize_t nread = read(serialfd, buffer, 512);
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
    }
  }
  else // 0 != cpid : Parent : stdin -> arduino
  {
    while (is_running)
    {
      ssize_t nread = fread(buffer, 1, 512, stdin);
      if (-1 == nread)
      {
        perror("read from stdin");
        exit(EXIT_FAILURE);
      }
      if (write(serialfd, buffer, nread) != nread)
      {
        perror("write to arduino");
        exit(EXIT_FAILURE);
      }
    }
    kill(cpid, SIGTERM);
  }

  return 0;
}