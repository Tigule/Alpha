#pragma once

class C3Color {
 public:
  C3Color() : b(0.0f), g(0.0f), r(0.0f) {
  }

  C3Color(float r, float g, float b) : b(b), g(g), r(r) {
  }

  float b;
  float g;
  float r;
};
