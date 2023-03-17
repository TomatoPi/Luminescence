#pragma once

#include "factory.h"

#include <list>
#include <stdexcept>
#include <unordered_map>

namespace opto {
namespace device {

  struct duplicated_device : std::runtime_error {
    duplicated_device(const std::string& str) : std::runtime_error(str) {}
  };

  class manager {
  public : 

    struct registered_devices : std::unordered_map<std::string, signature> {};
    struct pending_devices : std::list<std::string> {};
    struct active_devices : std::list<class device> {};
    struct failed_devices : std::list<std::string> {};

    void register_device(const signature& sig)
    {
      auto name = std::get<meta::name>(sig).value;
      if (_reg.find(name) != _reg.end())
        throw duplicated_device(name);
      _reg.emplace_hint(_reg.end(), name, sig);
      _builder.push(sig);
    }

    void update()
    {
      size_t max_retry = 5;
      for (size_t retry=0 ; retry < max_retry ; ++retry)
      {
        try {
          _builder.update();
        } catch (build_failure& e)
        {
          std::cerr << "[" << e.dev_name << "] - Failed build device : " << e.what() << '\n';
          std::cerr << "    Retry building ...\n";
          _builder.push(_reg[e.dev_name]);
          continue;
        }
        break;
      }
      _actives.splice(_actives.end(), _builder.pull());
    }

    template <typename Fn>
    void foreach_device(Fn fn) //, std::list<class device>& list, factory& builder)
    {
      for (auto itr = _actives.begin(); itr != _actives.end() ;)
      try {
        auto& dev = *itr;
        if (!dev.alive())
          throw std::runtime_error("Bad device running");
        fn(dev);
        ++itr;
      }
      catch (std::runtime_error& e)
      {
        std::cerr << "[" << std::get<meta::name>(static_cast<signature>(*itr)).value 
                  << "] - Failure on device : " << e.what() << '\n';
        std::cerr << "    Reload device ...\n";
        _builder.push(static_cast<signature>(*itr));
        itr = _actives.erase(itr);
      }
    }

  private :

    registered_devices _reg;
    active_devices  _actives;
    factory         _builder;
    failed_devices  _fails;
  };
}
}