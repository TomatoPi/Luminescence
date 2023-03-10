#pragma once

#include "transport.h"

#include <string>

namespace transport {
/// @brief Namespace holding Serial transport related objects
namespace serial {

  /* *** Serial Connection Properties *** */

  struct address {
    std::string port;
    int baudrate;
  };

  using serial_config = std::tuple<address>;

  /* *** Runtime Serial Transport Properties *** */

  struct runtime_config {
    size_t buffer_size;
  };

  /* *** Serial Transport Implementation *** */

  using serial_signature = std::tuple<serial_config, runtime_config>;

  class serial : public transport {
  public :

    /* *** Constructors *** */

    /// @brief tries to construct a valid socket object, throw on failure
    template <typename Sig = serial_signature>
    explicit serial(const Sig& sig)
    : serial(std::get<runtime_config>(sig), open(std::get<serial_config>(sig)))
    {}

    /// @brief construct an unbound socket
    constexpr serial() noexcept
    : serial(runtime_config(), 0)
    {}

    ~serial() { close(_fd); }

    /// @brief sockets are not copyable 
    serial(const serial&) = delete;
    serial& operator= (const serial&) = delete;

    /// @brief allows sockets to be moved from 
    serial(serial&& s) noexcept : serial(s._cfg, s._fd) { s._fd = 0; }
    serial& operator= (serial&& s)
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

  private:
    /// @brief delegated constructor 
    explicit constexpr serial(runtime_config cfg, int fd)
    : _cfg{cfg}, _fd{fd}
    {}

    /// @brief takes the string name of the serial port (e.g. "/dev/tty.usbserial","COM1")
    ///   and a baud rate (bps) and connects to that port at that speed and 8N1.
    ///   opens the port in fully raw mode so you can send binary data.
    /// @return a valid file descriptor, throws on failure
    static int open(const serial_config& addr);

    /// @brief Close the holded file descriptor if exists, throw on failure
    static void close(int fd);

    runtime_config _cfg;
    int _fd;
  };
}
}