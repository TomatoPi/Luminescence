#pragma once

#include "Arduino.h"

template <typename T>
class vec3 {
public:
    union { T x, r; };
    union { T y, g; };
    union { T z, b; };

    constexpr vec3(T x = 0, T y = 0, T z = 0)
        : x(x), y(y), z(z) {}

    void    set(const T* v) { x = v[0], y = v[1], z = v[2]; }
    void    set(T vx, T vy, T vz) { x = vx, y = vy, z = vz; }
    void    set(const vec3& v) { x = v.x, y = v.y, z = v.z; }

    vec3& operator+=(const vec3& v) {
      *this = *this + v;
      return *this;
    }
    vec3& operator-=(const vec3& v) {
      *this = *this - v;
      return *this;
    }
    constexpr vec3 operator/(T f) const
    {
      return vec3(x / f, y / f, z / f);
    }
    constexpr vec3 operator*(T f) const
    {
      return vec3(x * f, y * f, z * f);
    }

    vec3& operator/=(float f) {
      *this = *this / f;
      return *this;
    }
    vec3& operator*=(float f) {
      *this = *this * f;
      return *this;
    }

    void    zero() { x = 0, y = 0, z = 0; }
    float   length() { return sqrt(x * x + y * y + z * z); }
    operator T*() { return (T*)this; }

    T length() const { return sqrt(x * x + y * y + z * z); }
};

template <typename T>
inline vec3<T> cos(vec3<T> v)
{
  return vec3<T> (cos((float)v.x), cos((float)v.y), cos((float)v.z));
}

template <typename T>
boolean operator==(const vec3<T>& a, const vec3<T>& b) {
  return a.x == b.x && a.y == b.y && a.z == b.z; 
}
template <typename T>
boolean operator!=(const vec3<T>& a, const vec3<T>& b) {
  return !a == b;
}
template <typename T>
vec3<T>    operator+(const vec3<T>& a, const vec3<T>& b) {
  return vec3<T>(a.x + b.x, a.y + b.y, a.z + b.z);
}
template <typename T>
vec3<T> operator-(const vec3<T>& a, const vec3<T>& b) {
  return vec3<T> (a.x - b.x, a.y - b.y, a.z - b.z);
}
template <typename T>
vec3<T> operator*(const vec3<T>& a, const vec3<T>& b) {
  return vec3<T> (a.x * b.x, a.y * b.y, a.z * b.z);
}
