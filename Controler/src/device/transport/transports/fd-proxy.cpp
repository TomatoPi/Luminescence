#include "fd-proxy.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>   // strerror

namespace transport {
namespace fd {

packet_status write_to_fd(int fd, const packet_payload& p)
{
  if (fd <= 0)
    throw bad_file_descriptor();

  if (write(fd, p.data(), p.size()) != p.size())
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

opt_reply read_from_fd(int fd, size_t buffer_size)
{
  if (fd <= 0)
    throw bad_file_descriptor();

  std::string buffer;
  buffer.resize(buffer_size);

  ssize_t nread;
  if (-1 == (nread = read(fd, buffer.data(), buffer_size)))
  {
    int err = errno;
    if (err == EAGAIN || err == EWOULDBLOCK)
      return std::nullopt;
    else
      throw std::runtime_error(strerror(err));
  }
  buffer.resize(nread);

  if (buffer.size() == 0)
    throw std::runtime_error("Read failure");

  return std::make_optional<reply>(std::move(buffer));
}

}
}