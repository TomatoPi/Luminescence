/*
 * Copyright (C) 2021 DAGO Kokri Esaïe <dago.esaie@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

 ///
 ///  \file Midi.hpp
 /// 
 ///  \author DAGO Kokri Esaïe <dago.esaie@protonmail.com>
 ///  \date 2021-02-18
 ///

#pragma once

#include "ringbuffer.h"

namespace sfx
{
  namespace midi
  {
    enum Status : uint8_t
    {
      InvalidType = 0x00,
      NoteOff = 0x80,
      NoteOn = 0x90,
      AfterTouchPoly = 0xA0,
      ControlChange = 0xB0,
      ProgramChange = 0xC0,
      AfterTouchChannel = 0xD0,
      PitchBend = 0xE0,
      SystemExclusive = 0xF0,
      TimeCodeQuarterFrame = 0xF1,
      SongPosition = 0xF2,
      SongSelect = 0xF3,
      TuneRequest = 0xF6,
      Clock = 0xF8,
      Start = 0xFA,
      Continue = 0xFB,
      Stop = 0xFC,
      ActiveSensing = 0xFE,
      SystemReset = 0xFF,
    };

    // \brief Convert given status byte to a Status
    constexpr Status asStatus(uint8_t byte);

    /// \brief Return message length for given type
    uint8_t lengthof(Status status);

    /// \brief Return true if given status byte correspond to a realtime message
    constexpr bool isRealtime(Status status);

    struct StatusByte
    {
      constexpr StatusByte(uint8_t status = 0, uint8_t channel = 0) : status(status), channel(channel) {}
      uint8_t status = 0;
      uint8_t channel = 0;
    };
    /// \brief Split Status byte into (type,channel) for channelled messages
    ///   return (status, 0) for non channeled messages
    StatusByte split(uint8_t status);

    struct Event
    {
      uint8_t status;
      uint8_t channel;
      uint8_t d1;
      uint8_t d2;
    };
    struct RawEvent
    {
      uint8_t buffer[3];
      uint8_t length;
      explicit RawEvent(const Event& e);
      explicit RawEvent(uint8_t l = 0, uint8_t s = 0, uint8_t d1 = 0, uint8_t d2 = 0);
    };

    template <uint8_t RawBufferSize, uint8_t EventBufferSize>
    struct Parser
    {
      RingBuffer<Event, EventBufferSize> events;
      uint8_t rawbuffer[RawBufferSize];
      uint8_t cur_length, expected_length;

      void Init();
      void Parse(uint8_t byte);
      Event NextEvent();
      bool HasNext();
    };
  }
}

////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////

namespace sfx
{
  namespace midi
  {
    RawEvent::RawEvent(const Event& e) :
      buffer{ 0,0,0 }, length(midi::lengthof(asStatus(e.status)))
    {
      buffer[0] = e.status | e.channel;
      if (1 < length) buffer[1] = e.d1;
      if (2 < length) buffer[2] = e.d2;
    }
    RawEvent::RawEvent(uint8_t l, uint8_t s, uint8_t d1, uint8_t d2) :
      buffer{ s,d1,d2 }, length(l)
    {
    }

    template<uint8_t Rs, uint8_t Es>
    void Parser<Rs, Es>::Init()
    {
      memset(rawbuffer, 0, Rs);
      cur_length = expected_length = 0;
    }
    template<uint8_t Rs, uint8_t Es>
    void Parser<Rs, Es>::Parse(uint8_t byte)
    {
      if (0x80 & byte) { // is status
        Status status = asStatus(byte & 0xF0);
        if (isRealtime(status)) {
          Event event = { byte, 0 };
          events.add(event);
          return;
        }
        if (0 < cur_length) {
          /// TODO : Signal that messages are smashed
        }
        expected_length = lengthof(status);
        cur_length = 0;
      }

      if (Rs <= cur_length) {
        cur_length = expected_length = 0;
        /// TODO : Signal that we have droped a message
        return;
      }

      if (0 < expected_length) {
        rawbuffer[cur_length] = byte;
        cur_length += 1;
        if (cur_length == expected_length) {
          auto pair = split(rawbuffer[0]);
          uint8_t d1 = 1 < expected_length ? rawbuffer[1] : 0;
          uint8_t d2 = 2 < expected_length ? rawbuffer[2] : 0;
          Event event = { pair.status, pair.channel, d1, d2 };
          events.add(event);
          cur_length = expected_length = 0;
        }
      }
    }
    template<uint8_t Rs, uint8_t Es>
    Event Parser<Rs, Es>::NextEvent()
    {
      return events.pop();
    }
    template<uint8_t Rs, uint8_t Es>
    bool Parser<Rs, Es>::HasNext()
    {
      return !events.empty();
    }

    constexpr Status asStatus(uint8_t byte)
    {
      return static_cast<Status>(byte < 0xF0 ? byte & 0xF0 : byte);
    }
    uint8_t lengthof(Status status)
    {
      switch (status & 0xF0) {
      case 0xF0:
        switch (status) {
        case 0xF0: return 0;
        case 0xF2: return 3;
        case 0xF3: return 2;
        case 0xF6: return 1;
        case 0xF1: return 2;
        case 0xF8: //
        case 0xFA: //
        case 0xFB: //
        case 0xFC: //
        case 0xFE: //
        case 0xFF: return 1;
        default: //fprintf(stderr, "Invalid Midi Sys : %02x\n", type);
          return 0;
        }
      case 0x80: //
      case 0x90: //
      case 0xA0: //
      case 0xB0: //
      case 0xE0: return 3;
      case 0xD0: //
      case 0xC0: return 2;
      default: //fprintf(stderr, "Invalid Midi Type : %02x", type);
        return 0;
      }
    }
    constexpr bool isRealtime(Status status)
    {
      return Status::Clock == status
        || Status::Start == status
        || Status::Continue == status
        || Status::Stop == status;
    }
    StatusByte split(uint8_t status)
    {
      if (0xF0 <= status) {
        return StatusByte { status, 0 };
      } else {
        return StatusByte { status & 0xF0, status & 0x0F };
      }
    }
  }
}
