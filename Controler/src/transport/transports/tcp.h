#pragma once

#include "transport/transport.h"

#include <string>
#include <tuple>

namespace transport {
/// @brief Namespace holding TCP transport related objects
namespace tcp {

  /* *** TCP transport implementation *** */

  /// @brief Represent an opened tcp socket
  class socket : public transport {
  public :
  
    /* *** TCP Socket Properties *** */

    /// @brief Represents an IPv4 address using dotted notation
    struct address {
      std::string host;
      std::string port;
    };

    /// @brief Holds configuration for the linux socket
    struct keepalive_config {
      int counter = 100;
      int interval = 1;
    };
    
    /// @brief Holds configuration for reading and writing to the socket
    struct runtime_config {
      size_t buffer_size;
    };

    /// @brief Holds all parameters required to build a valid socket
    struct signature {
      address           addr;
      keepalive_config  kcfg;
      runtime_config    rcfg;
    };

    /* *** Constructors *** */

    /// @brief tries to construct a valid socket object, throw on failure
    explicit socket(const signature& sig)
    : socket(sig, open(sig.addr, sig.kcfg))
    {}

    /// @brief construct an unbound socket
    constexpr socket() noexcept
    : socket(signature(), 0)
    {}

    ~socket() { close(_fd); }

    /// @brief sockets are not copyable 
    socket(const socket&) = delete;
    socket& operator= (const socket&) = delete;

    /// @brief allows sockets to be moved from 
    socket(socket&& s) noexcept : socket(s._cfg, s._fd) { s._fd = 0; }
    socket& operator= (socket&& s)
    {
      if (s._fd != _fd && _fd != 0)
        close(_fd);

      _fd = s._fd;
      _cfg = s._cfg;
      s._fd = 0;

      return *this;
    };

    /* *** Methods *** */

    explicit constexpr operator bool() const noexcept
    { return _fd != 0; }

    /* *** Inherited from transport *** */
 
    constexpr bool alive() const noexcept override
    { return static_cast<bool>(*this); }

protected: 

    /// @brief Try to sent given packet over the tcp socket
    /// @param p packet to send
    /// @return packet_status::Failed on write failure
    /// @throws std::runtime_error if an unrecoverable error arise
    /// @throws unbound_socket if this method is called on a dead socket
    packet_status vsend(const packet_payload& p) override;

    /// @brief Try to read a message from the tcp socket
    /// @return A message if such exists, throw on unrecoverable error
    /// @throws std::runtime_error if an unrecoverable error arise
    /// @throws unbound_socket if this method is called on a dead socket
    opt_reply vreceive() override;

  private :

    /// @brief delegated constructor 
    explicit constexpr socket(const signature& cfg, int fd)
    : _cfg{cfg}, _fd{fd}
    {}

    /// @brief Try to open a linux tcp non blocking connection to given host
    /// @param addr address of the server to connect to
    /// @return opened socket's file descriptor on success, throw on failure
    static int open(const address& addr, const keepalive_config& kcfg);

    /// @brief Close the holded file descriptor if exists, throw on failure
    static void close(int fd);

    signature _cfg;
    int _fd;
  };
}
}