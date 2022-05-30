#include "jack-bridge.hpp"
#include "mapper.hpp"
#include "manager.hpp"
#include "arduino-bridge.hpp"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>

#include <thread>
#include <future>
#include <string>
#include <iostream>

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
    fprintf(stderr, "Usage : %s <setup-file> <save-file> <driver-ip> <driver-port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  JackBridge apc_bridge{"APC40-Bridge"};
  Mapper apc_mapper{Mapper::APC40_mappings()};
  Manager manager(argv[2], argv[1]);
  ArduinoBridge arduino(argv[3], argv[4]);

  apc_bridge.activate();

  std::future<std::optional<std::string>> input;

  while (is_running)
  {
    if (!input.valid())
    {
      input = std::async(std::launch::async, [&]() {
        std::cout << "Wait for arduino message" << '\n';
        return arduino.receive();
      });
    }
    else
    {
      std::future_status status = input.wait_for(std::chrono::microseconds(100));
      if (status == std::future_status::ready)
      {
        auto str = input.get();
        if (!str.has_value())
          std::cerr << "Failed retrieve value" << '\n';
        else
          fprintf(stderr, "Recieved from arduino : %s\n", str.value().c_str());
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
          arduino.send(ctrl->addr_offset, ctrl->to_raw_message());
        }
      }
    }
    usleep(100);
  }
  std::cout << "Shuting down program" << std::endl;
  arduino.kill();


  return 0;
}
