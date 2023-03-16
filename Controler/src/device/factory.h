#pragma once

#include "device.h"

#include <json/json.h>

#include <variant>
#include <future>
#include <optional>
#include <utility>
#include <list>

namespace opto {
namespace device {

  using pending_device = std::pair<signature, std::future<class device>>;
  using maybe_device = std::variant<class device, pending_device, signature>;
  using opt_device = std::optional<class device>;

  struct build_failure : std::runtime_error {
    build_failure(const std::string& str) 
    : std::runtime_error(str) {}
  };

  signature parse_signature(const Json::Value& root);

  class factory {
  public :

    void push(const signature& sig);
    [[nodiscard]] opt_device pull();
    
    void update();  

  private :

    std::list<pending_device> _build_list;
    std::list<device>         _ready_list;
  };

}
}