#pragma once

#include <mutex>

namespace utils {

template <typename T>
struct locked {
  struct proxy {
    std::lock_guard<std::mutex> _lock;
    T& ref;
  };

  proxy lock()
  { return proxy{std::lock_guard<std::mutex>(_mutex), _obj}; }

  std::mutex _mutex;
  T          _obj;
};

}