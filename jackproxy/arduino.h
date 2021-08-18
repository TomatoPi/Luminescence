#pragma once

#include "../driver/common.h"

#include <unordered_map>
#include <stdio.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <queue>

#include "arduino-serial-lib.h"
#include <termios.h>

class Arduino
{
public:
  using Queue = std::queue<std::pair<void*, SerialPacket>>;
  using Buffer = std::unordered_map<void*, SerialPacket>;

private:
  int fd = 0;
  std::thread serial_thread;
  Buffer buffer;
  Queue queue;
  bool kill = false;

  Buffer::iterator last_packet_sent;

  void wait_for_arduino()
  {
    char charbuffer[512];
    int res = 0;
    while (true)
    {
      memset(charbuffer, 0, 512);
      res = serialport_read_until(fd, charbuffer, STOP_BYTE, 512, 100);
      uint8_t* raw = (uint8_t*)charbuffer;
      if (res < 0)
      {
        if (kill) break;
        else continue;
      }
      else if (raw[0] == 0 && raw[1] == static_cast<uint8_t>(STOP_BYTE))
      {
        // fprintf(stderr, "RCV : %d : ACK\n", res);
        buffer.erase(last_packet_sent);
        return;
      }
      else if (raw[0] == static_cast<uint8_t>(STOP_BYTE))
      {
        // fprintf(stderr, "RCV : %d : START\n", res);
        return;
      }
      else if (*charbuffer)
      {
        fprintf(stderr, "RCV : %d : %s\n", res, charbuffer);
      }
    }
  }

  void try_send_packet()
  {
    if (buffer.empty())
    {
      serialport_writebyte(fd, STOP_BYTE);
    }
    else
    {
      last_packet_sent = buffer.begin();
      const auto& [_, packet] = *last_packet_sent;
      const uint8_t* raw = Serializer::bytestream(packet);
      for (size_t i = 0 ; i < SerialPacket::Size ; ++i)
      {
        uint8_t byte = raw[i];
        serialport_writebyte(fd, byte);
        // fprintf(stderr, "0x%02x ", byte); 
      }
      // fprintf(stderr, "\n");
    }
  }

public:
  Arduino(const char* port)
  {
    serial_thread = std::thread([&, this](){
      fd = serialport_init(port, B115200);
      if (fd < 0)
        throw std::runtime_error("Can't Open Serial Port");
      while (!kill)
      {
        while (!queue.empty())
        {
          auto& [key, packet] = queue.front();
          buffer.insert_or_assign(key, packet);
          queue.pop();
        }
        wait_for_arduino();
        try_send_packet();
      };
    });
  }

  template <typename T>
  void push(const T& obj, uint8_t flags = 0)
  {
    queue.emplace((void*)&obj, Serializer::serialize(obj, flags));
  }
};