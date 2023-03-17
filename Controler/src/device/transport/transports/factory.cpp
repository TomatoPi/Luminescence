#include "factory.h"

#include <json/json.h>

#include <utils/visitor.h>
#include <utils/json.h>

namespace transport {
namespace factory {

  using namespace utils;

  /* *** TCP Parser *** */

  tcp::signature parse_tcp(const Json::Value& root)
  {
    tcp::address addr{
      .host = get<Json::String>(root, "host"),
      .port = get<Json::String>(root, "port")
    };

    tcp::keepalive_config kcfg{
      .counter = get<Json::Int>(root, "counter"),
      .interval = get<Json::Int>(root, "interval")
    };

    return std::make_tuple(addr, kcfg);
  }

  Json::Value make_json(const tcp_signature& sig)
  {
    Json::Value root;
    root["protocol"] = Json::String("TCP");
    
    auto addr = std::get<tcp::address>(sig);
    root["args"]["host"] = Json::String(addr.host);
    root["args"]["port"] = Json::String(addr.port);

    auto kcfg = std::get<tcp::keepalive_config>(sig);
    root["args"]["counter"] = Json::Int(kcfg.counter);
    root["args"]["interval"] = Json::Int(kcfg.interval);

    auto fd_cfg = std::get<fd::read_buffer_size>(sig);
    root["read_buffer_size"] = Json::UInt64(fd_cfg.value);

    return root;
  }

  /* *** Serial Parser *** */

  serial::signature parse_serial(const Json::Value& sig)
  {
    serial::address addr{
      .port = get<Json::String>(sig, "port"),
      .baudrate = get<Json::Int>(sig, "baudrate")
    };

    return std::make_tuple(addr);
  }  
  
  Json::Value make_json(const serial_signature& sig)
  {
    Json::Value root;
    root["protocol"] = Json::String("SERIAL");
    
    auto addr = std::get<serial::address>(sig);
    root["args"]["port"] = Json::String(addr.port);
    root["args"]["baudrate"] = Json::Int(addr.baudrate);

    auto fd_cfg = std::get<fd::read_buffer_size>(sig);
    root["read_buffer_size"] = Json::UInt64(fd_cfg.value);

    return root;
  }

  dummy::signature_type parse_dummy(const Json::Value& sig)
  { return dummy::signature_type(); }
  
  Json::Value make_json(const dummy_signature& sig)
  {
    Json::Value root;
    root["protocol"] = Json::String("DUMMY");
    root["args"] = Json::Value();

    return root;
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
      else if (code.compare("DUMMY") == 0)
        return parse_dummy(args);
      else
        throw bad_json("Invalid protocol : '" + code + "'");
    }(protocol_code, get<const Json::Value& >(sig, "args"));
    
    return std::visit(visitor{
      [sig](const tcp::signature& s) -> transport_signature_type
      {        
        fd::read_buffer_size fd_cfg{
          .value = get<Json::UInt64>(sig, "read_buffer_size")
        };
        return std::tuple_cat(s, std::make_tuple(fd_cfg)); 
      },
      [sig](const serial::signature& s) -> transport_signature_type
      {
        fd::read_buffer_size fd_cfg{
          .value = get<Json::UInt64>(sig, "read_buffer_size")
        };
        return std::tuple_cat(s, std::make_tuple(fd_cfg));
      },
      [](const dummy::signature_type& s) -> transport_signature_type
      { return s; }
    }, protocol_sig);
  }

  Json::Value make_json(const transport_signature_type& sig)
  {
    return std::visit(visitor{
      [](const tcp_signature& s) -> auto { return make_json(s); },
      [](const serial_signature& s) -> auto { return make_json(s); },
      [](const dummy_signature& s) -> auto { return make_json(s); }
    }, sig);
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
        ,
        [](const dummy::signature_type& s)
        -> transport_ptr
        { return std::make_unique<dummy>(s); }
      }, sig);
  }
}
}