#include "jack-bridge.hpp"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

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
	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);

  if (argc != 3)
  {
    fprintf(stderr, "Usage : %s <driver-ip> <driver-port>", argv[0]);
    exit(EXIT_FAILURE);
  }

  JackBridge apc_bridge{"APC40-Bridge"};
  apc_bridge.activate();

  pid_t cpid = fork();
  if (-1 == cpid)
  {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (0 == cpid) // Child : read from stdin
  {
    char buff[512];
    while (is_running && fgets(buff, 512, stdin))
    {
      int tmp[3];
      ssize_t len = 0, nread=strlen(buff);
      fprintf(stderr, "Read %zd bytes from stdin : %s\n", nread, buff);
      int n = sscanf(buff, "%02x %02x %02x", &tmp[0], &tmp[1], &tmp[2]);
      if (n != 3)
        fprintf(stderr, "Invalid input %d\n", n);
      else
      {
        std::vector<uint8_t> msg;
        msg.push_back(tmp[0]);
        msg.push_back(tmp[1]);
        msg.push_back(tmp[2]);
        apc_bridge.send_midi(std::move(msg));
      }
    }
  }
  else
  {
    while (is_running)
    {
      auto messages = apc_bridge.incomming_midi();
      for (auto& msg : messages)
      {
        fprintf(stderr, "recieved %lu bytes : %02x %02x %02x\n", msg.size(), msg[0], msg[1], msg[2]);
      }
    }
    kill(cpid, SIGKILL);
  }


  return 0;
}