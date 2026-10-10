#include <Base/Base.h>

#include <math.h>

float LinearSmooth(float from, float to, float progress) {
  return (to - from) * progress + from;
}

float OrganicSmooth(float from, float to, float progress) {
  return (1.0f - (float)cos(PI * progress)) * 0.5f * (to - from) + from;
}
