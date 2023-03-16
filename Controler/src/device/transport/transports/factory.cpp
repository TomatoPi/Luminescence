#include "factory.h"

#include <json/json.h>

#include <utils/visitor.h>

namespace transport {
namespace factory {

  /* *** Helpers Functions *** */

  template <typename T>
  T get(const Json::Value& root, const std::string& key)
  {
    if (!root.isMember(key))
      throw bad_json("Missing key : '" + key + "'");
    return root[key].as<T>();
  }

  template <>
  const Json::Value& get<const Json::Value& >(const Json::Value& root, const std::string& key)
  {
    if (!root.isMember(key))
      throw bad_json("Missing key : '" + key + "'");
    return root[key];
  }

  /* *** TCP Parser *** */

  transport_signature_type parse_tcp(const Json::Value& sig)
  {
    if (!sig)
      throw bad_json("Missing 'args' keys");

    tcp::address addr{
      .host = get<Json::String>(sig, "host"),
      .port = get<Json::String>(sig, "port")
    };

    tcp::keepalive_config kcfg{
      .counter = get<Json::Int>(sig, "counter"),
      .interval = get<Json::Int>(sig, "interval")
    };

    return std::make_tuple(addr, kcfg);
  }

  /* *** Serial Parser *** */

  transport_signature_type parse_serial(const Json::Value& sig)
  {
    if (!sig)
      throw bad_json("Missing 'args' keys");

    serial::address addr{
      .port = get<Json::String>(sig, "port"),
      .baudrate = get<Json::Int>(sig, "baudrate")
    };

    return std::make_tuple(addr);
  }

  /* *** Root Parser *** */

  transport_signature_type parse_signature(const Json::Value& sig)
  {
    std::string protocol = get<Json::String>(sig, "protocol");
    
    if (protocol.compare("TCP") == 0)
      return parse_tcp(get<const Json::Value& >(sig, "args"));
    else if (protocol.compare("SERIAL") == 0)
      return parse_serial(get<const Json::Value& >(sig, "args"));
    else
      throw bad_json("Invalid protocol : '" + protocol + "'");
  }

  pending_transport open(const transport_signature_type& sig)
  {
    return std::visit(utils::visitor{
      [](const tcp::signature& s)
        -> pending_transport
        { 
          return std::async(std::launch::async, [](const tcp::signature& s)
            -> transport_ptr
            { return std::make_unique<tcp::socket>(s); }, s);
        }
        ,
      [](const serial::signature& s)
        -> pending_transport
        {
          return std::async(std::launch::async, [](const serial::signature& s)
            -> transport_ptr 
            { return std::make_unique<serial::serial>(s); }, s);
        }
    }, sig);
  }
}
}