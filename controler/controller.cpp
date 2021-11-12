#include "jack-bridge.hpp"
#include "mapper.hpp"
#include "manager.hpp"

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

  if (argc != 5)
  {
    fprintf(stderr, "Usage : %s <setup-file> <save-file> <driver-ip> <driver-port>", argv[0]);
    exit(EXIT_FAILURE);
  }

  JackBridge apc_bridge{"APC40-Bridge"};
  Mapper apc_mapper;
  Manager manager(argv[2], argv[1]);

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
      }
    }
    auto messages = apc_bridge.incomming_midi();
    for (auto& msg : messages)
    {
      auto commands = apc_mapper.midimsg_to_command(msg);
      for (auto& cmd : commands)
      {
        auto result = manager.process_command(cmd);
        for (auto& [ctrl, force] : result)
        {
          if (force || !(ctrl->flags & control_t::VOLATILE))
          {
            auto messages = apc_mapper.command_to_midimsg(ctrl->to_command_string());
            for (auto& msg : messages)
            {
              apc_bridge.send_midi(std::move(msg));
            }
          }
          auto arduino_cmd = ctrl->to_raw_message();
          fprintf(stderr, "To arduino : ");
          for (auto byte : arduino_cmd)
            fprintf(stderr, "%02x ", byte);
          fprintf(stderr, "\n");
        }
      }
    }
  }

  return 0;
}