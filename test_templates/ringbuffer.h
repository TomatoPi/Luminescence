#pragma once

template <typename T, unsigned int _BufferSize>
struct RingBuffer
{
  static constexpr const unsigned int BufferSize = _BufferSize;

  T array[BufferSize];
  unsigned int write_h = 0;
  unsigned int read_h = 0;

  const T& add(const T& obj)
  {
    array[write_h] = obj;
    write_h = (write_h +1) % BufferSize;
  }

  const T& pop()
  {
    const T& res = array[read_h];
    read_h = (read_h +1) % BufferSize;
    return res;
  }

  bool isFull() const
  {
    return (write_h + 1) % BufferSize == read_h;
  }

  bool empty() const
  {
    return write_h == read_h;
  }
};