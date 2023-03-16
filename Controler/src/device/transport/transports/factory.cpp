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

  tcp::signature parse_tcp(const Json::Value& sig)
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

  serial::signature parse_serial(const Json::Value& sig)
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
    std::string protocol_code = get<Json::String>(sig, "protocol");
    auto protocol_sig = [](const std::string& code, const Json::Value& args)
    -> proto_transport_signature {
      if (code.compare("TCP") == 0)
        return parse_tcp(args);
      else if (code.compare("SERIAL") == 0)
        return parse_serial(args);
      else
        throw bad_json("Invalid protocol : '" + code + "'");
    }(protocol_code, get<const Json::Value& >(sig, "args"));
    
    fd::read_buffer_size fd_cfg{
      .value = get<Json::UInt64>(sig, "read_buffer_size")
    };
    
    return std::visit([fd_cfg](auto psig) -> auto {
      return std::tuple_cat(psig, std::make_tuple(fd_cfg));
    }, protocol_sig);
  }

  transport_ptr open(const transport_signature_type& sig)
  {
    return std::visit(utils::visitor{
        [](const tcp::socket::signature_type& s)
        -> transport_ptr
        { return std::make_unique<tcp::socket>(s); }
        ,
        [](const serial::serial::signature_type& s)
        -> transport_ptr
        { return std::make_unique<serial::serial>(s); }
      }, sig);
  }
}
}