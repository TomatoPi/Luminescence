#pragma once

#include "device.h"

#include <json/json.h>

#include <variant>
#include <future>
#include <optional>
#include <memory>
#include <utility>
#include <list>

namespace opto {
namespace device {

  using device_ptr = std::unique_ptr<class device>;
  using pending_device = std::pair<signature, std::future<device_ptr>>;
  using maybe_device = std::variant<device_ptr, pending_device, signature>;

  struct build_failure : std::runtime_error {
    build_failure(const std::string& name, const std::string& str) 
    : std::runtime_error(str), dev_name(name) {}

    std::string dev_name;
  };

  signature parse_signature(const Json::Value& root);
  
  Json::Value make_json(const signature& sig);

  class factory {
  public :

    void push(const signature& sig);
    [[nodiscard]] std::list<device_ptr> pull() noexcept;
    
    void update();  

  private :

    std::list<pending_device> _build_list;
    std::list<device_ptr>     _ready_list;
  };

}
}