#include "device/manager.h"

#include <json/json.h>
#include <utils/json.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace opto::device;
using namespace transport::factory;

int main(int argc, char * const argv[])
{
  // if (argc != 2)
  // {
  //   std::cerr << "Usage : " << argv[0] << "TCP|SERIAL|filepath\n";
  //   return -1;
  // }
  std::ostringstream output;

  // if ("TCP" == argv[1])
  output << "{\n";
  output << "\"devices\" : {\n";
  {
    output << "\"TCP\" : ";
    signature def{
      meta::name{"default-tcp-device"},
      meta::transport{tcp_signature{}}};
    /***!< default constructed signature */
    auto root = make_json(def);
    output << root;
    output << std::endl;
    // return 0;
  }
  // else if ("SERIAL" == argv[1])
  output << ",\n";
  {
    output << "\"SERIAL\" : ";
    signature def{
      meta::name{"default-serial-device"},
      meta::transport{serial_signature{}}};
    /***!< default constructed signature */
    auto root = make_json(def);
    output << root;
    output << std::endl;
    // return 0;
  }
  output << ",\n";
  {
    output << "\"DUMMY\" : ";
    signature def{
      meta::name{"default-dummy-device"},
      meta::transport{transport::dummy::signature_type{}}};
    /***!< default constructed signature */
    auto root = make_json(def);
    output << root;
    output << std::endl;
    // return 0;
  }
  output << "}\n}";
  std::string device_json = output.str();
  std::cout << device_json << '\n';
  // else
  {
    std::istringstream stream(device_json);
    Json::Value root;
    stream >> root;

    std::vector<std::string> devices_codes = {"TCP", "SERIAL", "DUMMY"};
    std::vector<signature> devices_sigs;
    for (auto code : devices_codes)
      devices_sigs.emplace_back(opto::device::parse_signature(root["devices"][code]));

    manager devices_manager;
    for (auto sig : devices_sigs)
      devices_manager.register_device(sig);

    while (true)
    {
      devices_manager.update();
      devices_manager.foreach_device([](class device& dev) -> void {
          auto opt = dev.receive();
          if (opt.has_value())
          { 
            std::cout << "[" << std::get<meta::name>(static_cast<signature>(dev)).value 
                      << "] - Received : " << opt.value().content << '\n';
          }
      });
      devices_manager.foreach_device([](class device& dev) -> void {
          size_t N = 1;
          std::vector<uint8_t> bulk = {'O', 'p', 't', 'o', 0, 0, (uint8_t)(N*3), 0};
          for (size_t i=0 ; i<N ; ++i)
          {
            bulk.emplace_back(0);
            bulk.emplace_back(0);
            bulk.emplace_back(i*4);
          }

          switch (dev.send(bulk))
          {
            case transport::packet_status::Sent :
              std::this_thread::sleep_for(std::chrono::microseconds(100));
              // std::cout << "Packet sent\n";
              break;
            case transport::packet_status::Failed :
              std::cerr << "[" << std::get<meta::name>(static_cast<signature>(dev)).value 
                << "] - Failed send Packet to addr " << 0 << '\n';
              break;
          }
      });
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    /* *** While true - end *** */

    // // std::list<device> devices;
    // {
    //   // std::string dev = "SERIAL";
    //   // std::string dev = "DUMMY";

    //   factory builder;

    //   builder.push(sig);
    //   do {

    //     try {
    //       builder.update();
          
    //       auto newdevices = builder.pull();
    //       if (0 < newdevices.size())
    //       {
    //         for (auto& d : newdevices)
    //         {
    //           std::cout << "Successfully created new device : " << make_json(static_cast<signature>(d));
              
    //           if (!d.alive())
    //             throw std::runtime_error("Bad device returned");

    //           switch (d.send(std::vector<uint8_t>({'O', 'p', 't', 'o', 0, 0, 3, 0, 255, 255, 255})))
    //           {
    //             case transport::packet_status::Sent :
    //               std::this_thread::sleep_for(std::chrono::microseconds(1000));
    //               // std::cout << "Packet sent\n";
    //               break;
    //             case transport::packet_status::Failed :
    //               std::cerr << "[" << std::get<meta::name>(static_cast<signature>(d)).value 
    //                 << "] - Failed send Packet to addr " << 0 << '\n';
    //               break;
    //           }
    //         }
    //         devices.splice(devices.end(), std::move(newdevices));

    //       }
    //     }
    //     catch (std::runtime_error& e)
    //     {
    //       std::cout << "Failure : " << e.what() << '\n';
    //       continue;
    //     }

    //     for (auto itr = devices.begin(); itr != devices.end() ;)
    //     try {
    //       auto& dev = *itr;
    //       if (!dev.alive())
    //         throw std::runtime_error("Bad device running");

    //       size_t N = 1;
    //       std::vector<uint8_t> bulk = {'O', 'p', 't', 'o', 0, 0, (uint8_t)(N*3), 0};
    //       for (size_t i=0 ; i<N ; ++i)
    //       {
    //         bulk.emplace_back(0);
    //         bulk.emplace_back(0);
    //         bulk.emplace_back(i*4);
    //       }

    //       switch (dev.send(bulk))
    //       {
    //         case transport::packet_status::Sent :
    //           std::this_thread::sleep_for(std::chrono::microseconds(100));
    //           // std::cout << "Packet sent\n";
    //           break;
    //         case transport::packet_status::Failed :
    //           std::cerr << "[" << std::get<meta::name>(static_cast<signature>(dev)).value 
    //             << "] - Failed send Packet to addr " << 0 << '\n';
    //           break;
    //       }

    //       auto opt = dev.receive();
    //       if (opt.has_value())
    //       { 
    //         std::cout << "[" << std::get<meta::name>(static_cast<signature>(dev)).value 
    //                   << "] - Received : " << opt.value().content << '\n';
    //       }

    //       // std::cerr << "[" << std::get<meta::name>(static_cast<signature>(dev)).value 
    //       //           << "] - Done !\n";
    //       itr++;
    //       continue;
    //     }
    //     catch (std::runtime_error& e)
    //     {
    //       std::cerr << "[" << std::get<meta::name>(static_cast<signature>(*itr)).value 
    //                 << "] - Failure on device : " << e.what() << '\n';
    //       std::cerr << "Reload device ...\n";
    //       builder.push(static_cast<signature>(*itr));
    //       itr = devices.erase(itr);
    //     }

    //     // std::cout << "Nothing pending\n";
    //     std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    //   } while (true);
    // }
    // // try {
    // }
    // catch (utils::bad_json& e)
    // {
    //   std::cerr << "Failed parse Json File : " << e.what() << '\n';
    //   return -1;
    // }
  }
  
  return 0;
}