#include "jack-bridge.hpp"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>

#include <thread>
#include <future>
#include <string>

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

  std::future<std::string> input;

  while (is_running)
  {
    if (!input.valid())
    {
      input = std::async([&]() -> std::string {
        char buffer[512];
        fgets(buffer, 512, stdin);
        size_t len = strlen(buffer);
        if (0 < len && buffer[len-1] == '\n')
          buffer[len-1] = '\0';
        return std::string(buffer);
      });
    }
    else
    {
      std::future_status status = input.wait_for(std::chrono::microseconds(100));
      if (status == std::future_status::ready)
      {
        auto str = input.get();
        int tmp[3];
        fprintf(stderr, "Read %zd bytes from stdin : %s\n", str.size(), str.c_str());
        int n = sscanf(str.c_str(), "%02x %02x %02x", &tmp[0], &tmp[1], &tmp[2]);
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
    auto messages = apc_bridge.incomming_midi();
    for (auto& msg : messages)
    {
      fprintf(stderr, "recieved %lu bytes : %02x %02x %02x\n", msg.size(), msg[0], msg[1], msg[2]);
    }
  }

  return 0;
}