#include <math.h>

float __fastcall LinearSmooth(float from, float to, float progress) {
    // TODO: implement
    return 0;
}

float __fastcall OrganicSmooth(float from, float to, float progress) {
  return (1.0f - static_cast<float>(cos(3.1415927f * progress))) * 0.5f * (to - from) + from;
}
