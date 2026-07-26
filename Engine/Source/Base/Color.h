#pragma once

class C3Color {
 public:
  C3Color() : b(0.0f), g(0.0f), r(0.0f) {
  }

  C3Color(float r, float g, float b) : b(b), g(g), r(r) {
  }

  C3Color &operator+=(const C3Color &x) {
    b += x.b;
    g += x.g;
    r += x.r;
    return *this;
  }

  C3Color &operator+=(float x) {
    b += x;
    g += x;
    r += x;
    return *this;
  }

  C3Color &operator-=(const C3Color &x) {
    b -= x.b;
    g -= x.g;
    r -= x.r;
    return *this;
  }

  C3Color &operator-=(float x) {
    b -= x;
    g -= x;
    r -= x;
    return *this;
  }

  C3Color &operator*=(float x) {
    b *= x;
    g *= x;
    r *= x;
    return *this;
  }

  C3Color &operator/=(float x) {
    b /= x;
    g /= x;
    r /= x;
    return *this;
  }

  int operator==(const C3Color &x) {
    return b == x.b && g == x.g && r == x.r;
  }

  int operator!=(const C3Color &x) {
    return !(*this == x);
  }

  float b;
  float g;
  float r;
};
