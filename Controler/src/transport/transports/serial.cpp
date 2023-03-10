#include "serial.h"

#ifdef __unix__

#include <sys/ioctl.h>
#include <termios.h>  // POSIX terminal control definitions 

#endif

#include <stdio.h>    // Standard input/output definitions 
#include <unistd.h>   // UNIX standard function definitions 
#include <fcntl.h>    // File control definitions 
#include <errno.h>    // Error number definitions 
#include <string.h>   // String function definitions 

#include <stdexcept>

namespace opto:: inline transport {
namespace serial {

packet_status serial::vsend(const packet_payload& p)
{
  if (write(_fd, p.data(), p.size()) != p.size())
  {
    int err = errno;
    if (err == EAGAIN || err == EWOULDBLOCK)
      return packet_status::Failed;
    else
      throw std::runtime_error(strerror(err));
  }
  else
    return packet_status::Sent;
}

opt_reply serial::vreceive()
{
  std::string buffer;
  buffer.resize(_cfg.buffer_size);

  ssize_t nread;
  if (-1 == (nread = read(_fd, buffer.data(), _cfg.buffer_size)))
  {
    int err = errno;
    if (err == EAGAIN || err == EWOULDBLOCK)
      return std::nullopt;
    else
      throw std::runtime_error(strerror(err));
  }
  buffer.resize(nread);

  return std::make_optional<reply>(std::move(buffer));
}

#ifdef __unix__

/// @brief Try to open a linux tcp non blocking connection to given host
/// @param addr address of the server to connect to
/// @return opened socket's file descriptor on success, throw on failure
int serial::open(const serial_config& cfg)
{
  int fd = open(cfg.port.c_str(), O_RDWR | O_NONBLOCK );
  
  if (fd == -1)
  {
    int err = errno;
    throw std::runtime_error("serialport open : " + std::string(strerror(err)));
  }
  
  //int iflags = TIOCM_DTR;
  //ioctl(fd, TIOCMBIS, &iflags);     // turn on DTR
  //ioctl(fd, TIOCMBIC, &iflags);    // turn off DTR

  struct termios toptions;
  if (tcgetattr(fd, &toptions) < 0)
  {
    int err = errno;
    throw std::runtime_error("serialport open : tcgetattr : " + std::string(strerror(err)));
  }

  speed_t brate = cfg.baudrate;
  cfsetispeed(&toptions, brate);
  cfsetospeed(&toptions, brate);

  // 8N1
  toptions.c_cflag &= ~PARENB;
  toptions.c_cflag &= ~CSTOPB;
  toptions.c_cflag &= ~CSIZE;
  toptions.c_cflag |= CS8;
  // no flow control
  toptions.c_cflag &= ~CRTSCTS;

  //toptions.c_cflag &= ~HUPCL; // disable hang-up-on-close to avoid reset

  toptions.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
  toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

  toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
  toptions.c_oflag &= ~OPOST; // make raw

  // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
  toptions.c_cc[VMIN]  = 0;
  toptions.c_cc[VTIME] = 0;
  //toptions.c_cc[VTIME] = 20;
  
  tcsetattr(fd, TCSANOW, &toptions);
  if( tcsetattr(fd, TCSAFLUSH, &toptions) < 0)
  {
    int err = errno;
    throw std::runtime_error("serialport open : tcsetattr : " + std::string(strerror(err)));
  }

  return fd;
}

/// @brief Close the holded file descriptor if exists, throw on failure
void socket::close(int fd)
{
  if (fd != 0)
    ::close(fd);
}

#else // __unix __
#ifdef __WIN32__

int serial::open(const serial_config&) { return 0; }
void serial::close(int fd) {}

#endif // __WIN32__
#endif

}
}