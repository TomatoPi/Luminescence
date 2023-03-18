#include "factory.h"

#include "transport/transports/factory.h"

#include <utils/json.h>

#include <future>

namespace opto {
namespace device {

  using namespace utils;

  signature parse_signature(const Json::Value& root)
  {
    meta::name name{
      .value = get<Json::String>(root, "name")
    };

    meta::transport transport = transport::factory::parse_signature(
      get<const Json::Value&>(root, "transport")
      );

    return std::make_tuple(name, transport);
  }

  Json::Value make_json(const signature& sig)
  {
    Json::Value root;
    root["name"] = Json::String(std::get<meta::name>(sig).value);
    root["transport"] = transport::factory::make_json(std::get<meta::transport>(sig));

    return root;
  }
  
  void factory::push(const signature& sig)
  {
    _build_list.emplace_back(
      sig,
      std::async(std::launch::async, 
        [](const signature& s) -> auto { return std::make_unique<device>(s); }
        , sig)
      );
  }
  [[nodiscard]] std::list<device_ptr> factory::pull() noexcept
  {
    return std::move(_ready_list);
  }

  void factory::update()
  {
    for (auto itr = _build_list.begin() ; itr != _build_list.end() ;)
    try {
      auto& [sig, future] = *itr;
      if (!future.valid())
        throw std::runtime_error("Invalid Future");
        
      if (future.wait_for(std::chrono::microseconds(10)) != std::future_status::timeout)
      {
        auto device = future.get();
        _ready_list.emplace_back(std::move(device));
        itr = _build_list.erase(itr);
      }
      else
      {
        ++itr;
      }
    }
    catch (std::runtime_error& e)
    {
      auto name = std::get<meta::name>(itr->first);
      itr = _build_list.erase(itr);
      throw build_failure(name.value, e.what());
    }
  }
}
}