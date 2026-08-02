#include <Base/Base.h>

#include <math.h>

float LinearSmooth(float from, float to, float progress) {
  return (to - from) * progress + from;
}

float OrganicSmooth(float from, float to, float progress) {
  return (1.0f - static_cast<float>(cos(3.1415927f * progress))) * 0.5f * (to - from) + from;
}
