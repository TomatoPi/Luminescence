#pragma once

#include "../transport.h"

#include <stdexcept>
#include <tuple>

namespace transport {
namespace fd {

  /* *** File descriptor Properties *** */

  struct read_buffer_size { size_t value = 1024; };

  /* *** Exceptions *** */

  /// @brief Thrown on call to write or read with a null file descriptor
  struct bad_file_descriptor : std::exception {};

  /* *** Wrappers *** */

  /// @brief Write given packet to the file descriptor
  ///   Blocking or not depending on fd's parameters
  /// @param fd A Linux file descriptor to write to
  /// @param p  Datas array to send
  /// @return Sent on success, Failed on minor failure, throws on severe error
  packet_status write_to_fd(int fd, const packet_payload& p);

  /// @brief Read from given file descriptor
  /// @param fd A Linux file descriptor to read from
  /// @return A message from the device if available
  opt_reply read_from_fd(int fd, size_t buffer_size);

  /* *** Base Template class for transports based file descriptor *** */

  template <
    typename Signature,
    typename Opener, Opener open,
    typename Closer, Closer close
    >
  class proxy : public transport {
  public :

    using signature_type =  decltype(std::tuple_cat(std::declval<Signature>(), std::tuple<read_buffer_size>()));

    /* *** Constructors *** */

    /// @brief tries to construct a valid fd object, throw on failure
    explicit proxy(const signature_type& sig)
    : proxy(sig, open(sig))
    {}

    /// @brief construct an unbound file descriptor
    constexpr proxy() noexcept
    : proxy(signature_type(), 0)
    {}

    /// @brief File descriptor is closed on object's destruction
    ~proxy() { close(_fd); }

    /// @brief File descriptors are not copyable 
    proxy(const proxy&) = delete;
    proxy& operator= (const proxy&) = delete;

    /// @brief allows fds to be moved from 
    proxy(proxy&& s) noexcept : proxy(s._cfg, s._fd) { s._fd = 0; }
    proxy& operator= (proxy&& s)
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

    const signature_type& signature() const noexcept
    { return _cfg; }

    /* *** Inherited from transport *** */
 
    constexpr bool alive() const noexcept override
    { return static_cast<bool>(*this); }

  protected: 

    /// @brief Try to sent given packet over the tcp socket
    /// @param p packet to send
    /// @return packet_status::Failed on write failure
    /// @throws std::runtime_error if an unrecoverable error arise
    packet_status vsend(const packet_payload& p) override
    { return write_to_fd(_fd, p); }

    /// @brief Try to read a message from the tcp socket
    /// @return A message if such exists, throw on unrecoverable error
    /// @throws std::runtime_error if an unrecoverable error arise
    opt_reply vreceive() override
    { return read_from_fd(_fd, std::get<read_buffer_size>(_cfg).value); }

  private:
    /// @brief delegated constructor 
    explicit constexpr proxy(signature_type cfg, int fd)
    : _cfg{cfg}, _fd{fd}
    {}

    signature_type  _cfg;
    int             _fd;
  };

}
}