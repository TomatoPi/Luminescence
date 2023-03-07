#pragma once

#include <mutex>

namespace utils {

template <typename T>
struct locked {
  struct proxy {
    std::lock_guard<std::mutex> _lock;
    T& _obj;
  };

  proxy lock()
  { return proxy{std::lock_guard<std::mutex>(_obj_mutex), _obj}; }

  std::mutex _obj_mutex;
  T          _obj;
};

}