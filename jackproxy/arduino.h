#pragma once

#include "../driver/common.h"

#include <unordered_map>
#include <stdio.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <queue>
#include <mutex>

#include "arduino-serial-lib.h"
#include <termios.h>

class Arduino
{
public:
  using Buffer = std::unordered_map<void*, std::pair<unsigned long, SerialPacket>>;

private:
  int fd = 0;
  std::thread serial_thread;
  std::mutex buffer_lock;
  Buffer buffer;
  bool kill = false;
  uint8_t index = 0;

  Buffer::iterator last_packet_sent;
  unsigned long last_sent_id = 0;
  unsigned long next_packet_id = 0;

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
        std::scoped_lock<std::mutex> _(buffer_lock);
        if (last_packet_sent->second.first == last_sent_id)
        {
          // fprintf(stderr, "Packet Sent OK : %lu %p\n", last_packet_sent->second.first, last_packet_sent->first);
          buffer.erase(last_packet_sent);
        }
        else
        {
          // fprintf(stderr, "Packet Keeped : %lu : %lu %p\n", last_sent_id, last_packet_sent->second.first, last_packet_sent->first);
        }
        return;
      }
      else if (raw[0] == static_cast<uint8_t>(STOP_BYTE))
      {
        // fprintf(stderr, "RCV : %d : START\n", res);
        return;
      }
      else if (*charbuffer)
      {
        fprintf(stderr, "[%d] RCV : %d : %s", index, res, charbuffer);
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
      const auto& [_, pair] = *last_packet_sent;
      const auto& [id, packet] = pair;
      last_sent_id = id;
      // fprintf(stderr, "%lu Pending Packets\n", buffer.size());
      // fprintf(stderr, "Send Packet : %lu %p\n", id, _);
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
  Arduino(const char* port, uint8_t index)
  {
    this->index = index;
    serial_thread = std::thread([this](const char* port){
      fd = serialport_init(port, B115200);
      if (fd < 0)
        throw std::runtime_error("Can't Open Serial Port : ");
      while (!kill)
      {
        wait_for_arduino();
        try_send_packet();
      };
    }, port);
  }

  template <typename T>
  void push(const T& obj, uint8_t flags = 0)
  {
    std::scoped_lock<std::mutex> _(buffer_lock);
    auto itr = buffer.insert_or_assign((void*)&obj, 
      std::make_pair(
        next_packet_id++,
        Serializer::serialize(obj, flags)));
    // fprintf(stderr, "Added Packet : %lu %p\n", itr.first->second.first, itr.first->first);
  }
};