#pragma once

template <typename T, unsigned int _SizeMax>
struct Array
{
  static constexpr const unsigned int SizeMax = _SizeMax;

  T array[SizeMax];
  unsigned int size = 0;

  const T& add(const T& obj) 
  {
    if (size < SizeMax)
    {
      array[size] = obj;
      size++;
    }
    return obj;
  }
  const T& add(T&& obj) 
  {
    if (size < SizeMax)
    {
      array[size] = obj;
      size++;
    }
    return array[size-1];
  }

  void clear() { size = 0; }

  const T& operator[] (unsigned int i) const { return array[i]; }
  T& operator[] (unsigned int i) { return array[i]; }
};

template <typename T, unsigned int _Size>
struct StaticArray
{
  static constexpr const unsigned int Size = _Size;
  T array[Size];

  const T& operator[] (unsigned int i) const { return array[i]; }
  T& operator[] (unsigned int i) { return array[i]; }
};
