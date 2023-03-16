#include "factory.h"

#include "transport/transports/factory.h"

#include <future>

namespace opto {
namespace device {

  signature parse_signature(const Json::Value& root)
  {

  }
  
  void factory::push(const signature& sig)
  {
    _build_list.emplace_back(
      sig,
      std::async(std::launch::async, 
        [](const signature& s) -> auto { return device(s); }
        , sig)
      );
  }
  [[nodiscard]] opt_device factory::pull()
  {
    if (_ready_list.empty())
      return std::nullopt;

    auto&& dev = std::move(_ready_list.front());
    _ready_list.pop_front();
    return std::make_optional<class device>(std::forward<decltype(dev)>(dev));
  }

  void factory::update()
  {
    for (auto itr = _build_list.begin() ; itr != _build_list.end() ;)
    try {
      auto& [sig, future] = *itr;
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
      throw build_failure(std::get<meta::name>(itr->first).value + " : " + e.what());
    }
  }
}
}