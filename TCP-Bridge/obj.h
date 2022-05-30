#pragma once

#include <unordered_set>

template <typename T>
class InstancedObject
{
public:
  using Instances = std::unordered_set<T*>;

  InstancedObject() { _instances.emplace(this); }
  virtual ~InstancedObject() { _instances.erase(this); }

  InstancedObject(const InstancedObject&) = delete;
  InstancedObject& operator= (const InstancedObject&) = delete;

  static Instances& Get() { return _instances; }

private:
  static Instances _instances;
};

template <typename T>
InstancedObject<T>::Instances _instances = InstancedObject<T>::Instances();
