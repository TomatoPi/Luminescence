#include "device.h"

#include "utils/visitor.h"

using namespace transport;

/// @brief Main namespace for optopoulpe top objects
namespace opto {
namespace device {

  // bool device::is_available() const noexcept
  // {
  //   return std::visit(utils::visitor{
  //     [](const transport_ptr& ptr) { return ptr && ptr->alive(); },
  //     [](const pending_transport&) { return false; }
  //     },
  //     _transport);
  // }

  // bool device::make_available()
  // {
  //   if (is_available())
  //     return true;
  
  //   std::visit(utils::visitor{
  //     [this](const transport_ptr&) 
  //     { _transport = open(std::get<meta::transport>(_cfg)); }
  //     ,
  //     [this](pending_transport& future)
  //     {
  //       if (future.wait_for(std::chrono::microseconds(10)) != std::future_status::timeout)
  //       {
  //         auto ptr = future.get();
  //         _transport = std::move(ptr);
  //       }
  //     }
  //     }, _transport);
    
  //   return is_available();
  // }

  // packet_status device::send(const packet_payload& p)
  // {
  //   if (is_available())
  //     return std::get<transport_ptr>(_transport)->send(p);
  //   else
  //     return packet_status::Failed;
  // }

  // opt_reply device::receive()
  // {
  //   if (is_available())
  //     return std::get<transport_ptr>(_transport)->receive();
  //   else
  //     return std::nullopt;
  // }
}
}