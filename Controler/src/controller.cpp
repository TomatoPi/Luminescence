/// @file MAIN

#include "midi/bridges/jack-bridge.h"
#include "midi/bridges/dummy.h"

#include "mapper.hpp"
#include "manager.hpp"

#include "device/manager.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>

#include <thread>
#include <future>
#include <string>
#include <iostream>

#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>

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

// std::vector<opto::device> parse_drivers(const std::filesystem::path& path)
// {
//   std::cout << "Read devices from : " << path << '\n';
//   std::vector<opto::device> devices;
//   std::ifstream stream(path);
//   for (std::array<char, 256> buff; stream.getline(&buff[0], 256, '\n');)
//   try {
//     if (buff[0] == '#')
//       continue;

//     std::cout << "Parsing new device : " << &buff[0] << '\n';

//     std::istringstream input(&buff[0]);
//     auto getline = [&](){
//       std::array<char, 256> tmp;
//       if (!input.getline(&tmp[0], 256, ' '))
//         throw std::string("Ill formed line : '") + std::string(&buff[0]);
//       return std::string(&tmp[0]);
//     };

//     opto::device_config cfg{getline()};
//     std::cout << "  Device name : " << cfg.name << '\n';

//     auto protocol_signature = [&]()
//     -> opto::transport_signature_type {
//       std::string protocol{getline()};
//       if (protocol.compare("TCP") == 0)
//       {
//         std::cout << "  TCP Protocol : " << cfg.name << '\n';

//         std::string host{getline()};
//         std::string port{getline()};

//         opto::tcp::address addr{host, port};
//         std::cout << "    Host=" << addr.host << " Port=" << addr.port << '\n';

//         opto::tcp::keepalive_config kcfg;
//         input >> kcfg.counter;
//         input >> kcfg.interval;
//         std::cout << "    KeepAlive : Counter=" << kcfg.counter << " Interval=" << kcfg.interval << '\n';

//         opto::tcp::socket_config socket_config{addr, kcfg};

//         opto::tcp::runtime_config rcfg;
//         input >> rcfg.buffer_size;
//         std::cout << "    Receive : Buffersize=" << rcfg.buffer_size << '\n';

//         return opto::tcp::socket_signature{socket_config, rcfg};
//       }
//       else if (protocol.compare("SERIAL") == 0)
//       {
//         std::cout << "  Serial Protocol : " << cfg.name << '\n';

//         std::string port{getline()};
//         int baudrate;
//         input >> baudrate;

//         opto::serial::address addr{port, baudrate};
//         std::cout << "    Port=" << addr.port << " Speed=" << addr.baudrate << '\n';

//         opto::serial::serial_config serial_config{addr};

//         opto::serial::runtime_config rcfg;
//         input >> rcfg.buffer_size;
//         std::cout << "    Receive : Buffersize=" << rcfg.buffer_size << '\n';

//         return opto::serial::serial_signature{serial_config, rcfg};
//       }
//       else
//         throw std::string("Invalid protocol : ") + protocol;
//     }();

//     std::cout << "  Openning device ...\n";
//     opto::device_signature device_signature{cfg, protocol_signature};
//     opto::device newdevice(device_signature);
    
//     devices.emplace_back(std::move(newdevice));
//     std::cout << "Done !\n";
//   }
//   catch (std::string what)
//   {
//     std::cerr << what << '\n';
//     continue;
//   }
//   return devices;
// }
opto::device::manager parse_devices(const std::filesystem::path& path)
{
  std::ifstream fstream(path);
  Json::Value root;
  fstream >> root;

  std::vector<opto::device::signature> devices_sigs;
  const auto& devices = root["devices"];
  for (int i=0 ; i<devices.size() ; ++i)
    devices_sigs.emplace_back(opto::device::parse_signature(devices[i]));

  opto::device::manager devices_manager;
  for (auto sig : devices_sigs)
    devices_manager.register_device(sig);
  return devices_manager;
}

int main(int argc, char* const argv[])
{
	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);

  // if (argc != 4)
  // {
  //   fprintf(stderr, "Usage : %s <setup-file> <save-file> <drivers-file>\n", argv[0]);
  //   exit(EXIT_FAILURE);
  // }

  #ifdef __unix__
  midi::jack::JackBridge apc_bridge{"APC40-Bridge"};
  #else
  midi::dummy_bridge apc_bridge;
  #endif

  Mapper apc_mapper{Mapper::APC40_mappings()};
  Manager manager(
    "/home/sfxd/Documents/2.0/progs/Optopoulpe/config/save.txt", 
    "/home/sfxd/Documents/2.0/progs/Optopoulpe/config/setup.txt"); // (argv[2], argv[1]);
  
  auto devices_manager{parse_devices("/home/sfxd/Documents/2.0/progs/Optopoulpe/config/devices.json")}; // argv[3])};

  apc_bridge.activate();

  usleep(1'000'000);

  std::cout << "Controller initialised, entering main loop\n";
  while (is_running)
  {
    devices_manager.update();

    devices_manager.foreach_device([](opto::device::device_ptr& dev) -> void {
        do {
          auto opt = dev->receive();
          if (!opt.has_value())
            break;

          std::cout << "[" << std::get<opto::device::meta::name>(static_cast<opto::device::signature>(*dev)).value 
                    << "] - Received : " << opt.value().content << '\n';
        } while (true);
    });
    // for (auto& device : devices)
    // {
    //   if (!device.alive())
    //   {
    //     // std::cerr << "[" << device.config().name << "] - Read : " << "Bad device\n";
    //     continue;
    //   }
        
    //   auto input = device.receive();
    //   if (!input.has_value())
    //     continue;
    //   std::cout << "[" << device.config().name << "] - " << input.value().content << '\n';
    // }
    
    std::unordered_map<size_t, std::vector<uint8_t>> bulk;

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
          bulk.emplace(ctrl->addr_offset, ctrl->to_raw_message());
        }
      }
    }

    // bulk.emplace(0, std::vector<uint8_t>({'O', 'p', 't', 'o', 0, 0, 0, 3, 255, 255, 255}));
    devices_manager.foreach_device(
      [&bulk](opto::device::device_ptr& dev)
      -> void 
      {
        for (auto& [addr, payload] : bulk)
        {
          dev->send(payload);
          std::this_thread::sleep_for(std::chrono::microseconds(1000));
        }
      });
    // {
    //   for (auto& device : devices)
    //   {
    //     if (!device.alive())
    //     {
    //       // std::cerr << "[" << device.config().name << "] - Write : " << "Bad device\n";
    //       continue;
    //     }

    //     switch (device.send(payload))
    //     {
    //       case opto::packet_status::Sent :
    //         std::this_thread::sleep_for(std::chrono::microseconds(1000));
    //         continue;
    //       case opto::packet_status::Failed :
    //         std::cerr << "[" << device.config().name << "] - Failed send Packet to addr " << addr << '\n';
    //         continue;
    //     }
    //   }
    // }
    std::this_thread::sleep_for(std::chrono::microseconds(10000));
  }
  std::cout << "Shuting down program" << std::endl;

  return 0;
}
