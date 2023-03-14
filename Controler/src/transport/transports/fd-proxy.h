#pragma once

namespace transport {

  template <
    typename Sig,
    typename Opener, Opener open,
    typename Closer, Closer close
    >
  class fd_proxy : public transport {
  public :

    /* *** Constructors *** */

    /// @brief tries to construct a valid socket object, throw on failure
    explicit fd_proxy(const Sig& sig)
    : fd_proxy(sig, open(sig))
    {}

    /// @brief construct an unbound socket
    constexpr serial() noexcept
    : serial(Sig(), 0)
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

    Sig _cfg;
    int _fd;
  };
}