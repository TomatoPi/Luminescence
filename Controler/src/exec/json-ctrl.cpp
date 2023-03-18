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
  // else
  {
    std::istringstream stream(device_json);
    Json::Value root;
    stream >> root;


    std::ifstream file("/home/sfxd/Documents/2.0/progs/Optopoulpe/config/due.json");
    file >> root["devices"]["FILE"];

    std::cout << root << '\n';

    std::vector<std::string> devices_codes = {"TCP", "SERIAL", "FILE"};
    std::vector<signature> devices_sigs;
    for (auto code : devices_codes)
      devices_sigs.emplace_back(opto::device::parse_signature(root["devices"][code]));

    manager devices_manager;
    for (auto sig : devices_sigs)
      devices_manager.register_device(sig);

    while (true)
    {
      devices_manager.update();
      devices_manager.foreach_device([](device_ptr& dev) -> void {
          auto opt = dev->receive();
          if (opt.has_value())
          { 
            std::cout << "[" << std::get<meta::name>(static_cast<signature>(*dev)).value 
                      << "] - Received : " << opt.value().content << '\n';
          }
      });
      devices_manager.foreach_device([](device_ptr& dev) -> void {
          size_t N = 1;
          std::vector<uint8_t> bulk = {'O', 'p', 't', 'o', 0, 0, (uint8_t)(N*3), 0};
          for (size_t i=0 ; i<N ; ++i)
          {
            bulk.emplace_back(0);
            bulk.emplace_back(0);
            bulk.emplace_back(i*4);
          }
          dev->send(bulk);
          // switch ()
          // {
          //   case transport::packet_status::Sent :
          //     std::this_thread::sleep_for(std::chrono::microseconds(100));
          //     // std::cout << "Packet sent\n";
          //     break;
          //   case transport::packet_status::Failed :
          //     std::cerr << "[" << std::get<meta::name>(static_cast<signature>(dev)).value 
          //       << "] - Failed send Packet to addr " << 0 << '\n';
          //     break;
          // }
      });
      
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    /* *** While true - end *** */
  }
  
  return 0;
}