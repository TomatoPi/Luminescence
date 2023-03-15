#include "factory.h"

#include "transports/tcp.h"
#include "transports/serial.h"

namespace transport {
namespace factory {

  /* *** Helpers types *** */

  using transport_signature_type = std::variant<
    tcp::signature,
    serial::signature
    >;

  template <typename T>
  T get(const Json::Value& root, const std::string& key)
  {
    if (!root.isMember(key))
      throw bad_json("Missing key : '" + key + "'");
    return root[key].as<T>();
  }

  template <>
  const Json::Value& get(const Json::Value& root, const std::string& key)
  {
    if (!root.isMember(key))
      throw bad_json("Missing key : '" + key + "'");
    return root[key];
  }

  pending_transport open_tcp(const Json::Value& sig)
  {
    if (!sig)
      throw bad_json("Missing 'args' keys");

    tcp::address address;
    
  }
  pending_transport open_serial(const Json::Value& sig)
  {

  }

  pending_transport open(const Json::Value& sig)
  {
    std::string protocol = get<Json::String>(sig, "protocol");
    if (protocol.compare("TCP") == 0)
      return open_tcp(sig["args"]);
    else if (protocol.compare("SERIAL") == 0)
      return open_serial(sig["args"]);
    else
      throw bad_json("Invalid protocol : '" + protocol + "'");
  }
}
}