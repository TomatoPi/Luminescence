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
    build_failure(const std::string& name, const std::string& str) 
    : std::runtime_error(str), dev_name(name) {}

    std::string dev_name;
  };

  signature parse_signature(const Json::Value& root);
  
  Json::Value make_json(const signature& sig);

  class factory {
  public :

    void push(const signature& sig);
    [[nodiscard]] std::list<device> pull() noexcept;
    
    void update();  

  private :

    std::list<pending_device> _build_list;
    std::list<device>         _ready_list;
  };

}
}