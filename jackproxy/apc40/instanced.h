#pragma once

#include<array>
#include <cassert>

template <typename T>
class MonoInstanced
{
private:
  static T* _instance;

public:

  MonoInstanced() { assert(nullptr == _instance); _instance = (T*)this;}

  MonoInstanced(const MonoInstanced&) = delete;
  MonoInstanced& operator= (const MonoInstanced&) = delete;

  static T* Get() { return _instance; }
};
template <typename T> T* MonoInstanced<T>::_instance = nullptr;


template <typename T, size_t N>
class ArrayInstanced
{
public:
  using Instances = std::array<T*, N>;

private:
  static Instances _instances;
  size_t index = 0;

public:

  template <typename Builder>
  static void Generate(const Builder& builder)
  {
    for (size_t i = 0 ; i < N ; ++i)
      builder(i);
  }

  ArrayInstanced(size_t i) : index(i) {
    assert(nullptr == _instances[i]);
    _instances[i] = (T*)this;
  }

  ArrayInstanced(const ArrayInstanced&) = delete;
  ArrayInstanced& operator= (const ArrayInstanced&) = delete;

  size_t getIndex() const { return index; }

  static Instances& Get() { return _instances; }
  static T* Get(size_t i) { return _instances[i]; }
};
template <typename T, size_t N> typename ArrayInstanced<T,N>::Instances ArrayInstanced<T,N>::_instances = ArrayInstanced<T,N>::Instances();


template <typename T, size_t Cols, size_t Rows>
class D2ArrayInstanced
{
public:
  using Column = std::array<T*, Rows>;
  using Instances = std::array<Column, Cols>;

private:
  static Instances _instances;
  size_t col, row;

public:

  template <typename Builder>
  static void Generate(const Builder& builder)
  {
    for (size_t col = 0 ; col < Cols ; ++col)
      for (size_t row = 0 ; row < Rows ; ++row)
        builder(col, row);
  }

  D2ArrayInstanced(size_t col, size_t row) : col(col), row(row) {
    assert(nullptr == _instances[col][row]);
    _instances[col][row] = (T*)this;
  }

  D2ArrayInstanced(const D2ArrayInstanced&) = delete;
  D2ArrayInstanced& operator= (const D2ArrayInstanced&) = delete;

  size_t getCol() const { return col; }
  size_t getRow() const { return row; }

  static Instances& Get() { return _instances; }
  static Column& Get(size_t col) { return _instances[col]; }
  static T* Get(size_t col, size_t row) { return _instances[col][row]; }
};
template <typename T, size_t Cols, size_t Rows> typename D2ArrayInstanced<T,Cols,Rows>::Instances D2ArrayInstanced<T,Cols,Rows>::_instances = D2ArrayInstanced<T,Cols,Rows>::Instances();


template <typename T, size_t Banks, size_t Cols, size_t Rows>
class D3ArrayInstanced
{
public:
  using Column = std::array<T*, Rows>;
  using Bank = std::array<Column, Cols>;
  using Instances = std::array<Bank, Banks>;

private:
  static Instances _instances;
  size_t bank, col, row;

public:

  template <typename Builder>
  static void Generate(const Builder& builder)
  {
    for (size_t bank = 0 ; bank < Banks ; ++bank)
      for (size_t col = 0 ; col < Cols ; ++col)
        for (size_t row = 0 ; row < Rows ; ++row)
          builder(bank, col, row);
  }

  D3ArrayInstanced(size_t bank, size_t col, size_t row) :
    bank(bank), col(col), row(row)
  {
    assert(nullptr == _instances[bank][col][row]);
    _instances[bank][col][row] = (T*)this;
  }

  D3ArrayInstanced(const D3ArrayInstanced&) = delete;
  D3ArrayInstanced& operator= (const D3ArrayInstanced&) = delete;

  size_t getBank() const { return col; }
  size_t getCol() const { return col; }
  size_t getRow() const { return row; }

  static Instances& Get() { return _instances; }
  static Bank& Get(size_t bank) { return _instances[bank]; }
  static Column& Get(size_t bank, size_t col) { return _instances[bank][col]; }
  static T* Get(size_t bank, size_t col, size_t row) { return _instances[bank][col][row]; }
};
template <typename T, size_t Banks, size_t Cols, size_t Rows> typename D3ArrayInstanced<T,Banks,Cols,Rows>::Instances D3ArrayInstanced<T,Banks,Cols,Rows>::_instances = D3ArrayInstanced<T,Banks,Cols,Rows>::Instances();