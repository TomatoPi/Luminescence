#pragma once

#include <json/json.h>
#include <stdexcept>

namespace utils {

  /* *** Exceptions *** */

  /// @brief thrown if an ill-formed json is passed to parse_signature
  struct bad_json : std::runtime_error {
    explicit bad_json(const std::string& str)
    : std::runtime_error(str)
    {}
  };

  /* *** Helpers Functions *** */

  template <typename T>
  static T get(const Json::Value& root, const std::string& key)
  {
    if (!root)
      throw bad_json("Null Json");
    if (!root.isMember(key))
      throw bad_json("Missing key : '" + key + "'");
    return root[key].as<T>();
  }

  template <>
  const Json::Value& get<const Json::Value& >(const Json::Value& root, const std::string& key)
  {
    if (!root)
      throw bad_json("Null Json");
    if (!root.isMember(key))
      throw bad_json("Missing key : '" + key + "'");
    return root[key];
  }
}