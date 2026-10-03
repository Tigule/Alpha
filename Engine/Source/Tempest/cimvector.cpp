#include <Base/Base.h>

#include "Tempest/cimvector.h"

#include "Tempest/c3vector.h"

namespace NTempest {

  void RGBtoHSV(const C3Vector &rgb, C3Vector &hsv) {
    UINT max = rgb.MajorAxis();
    UINT min = rgb.MinorAxis();

    hsv[2] = rgb[max];
    hsv[1] = (hsv[2] != 0.0f) ? (rgb[max] - rgb[min]) / rgb[max] : 0.0f;

    if (hsv[1] == 0.0f) {
      hsv[0] = -1.0f;
      return;
    }

    float delta = rgb[max] - rgb[min];
    switch (max) {
      case 0:
        hsv[0] = (rgb[1] - rgb[2]) / delta;
        break;

      case 1:
        hsv[0] = 2.0f + (rgb[2] - rgb[0]) / delta;
        break;

      case 2:
        hsv[0] = 4.0f + (rgb[0] - rgb[1]) / delta;
        break;
    }

    hsv[0] *= 60.0f;
    if (hsv[0] < 0.0f) {
      hsv[0] += 360.0f;
    }
  }

  void HSVtoRGB(const C3Vector &hsv, C3Vector &rgb) {
    if (hsv[1] == 0.0f) {
      rgb = C3Vector(hsv[2], hsv[2], hsv[2]);
      return;
    }

    float h = hsv[0];
    if (h == 360.0f) {
      h = 0.0f;
    }
    h /= 60.0f;

    int   i = CMath::ftol_0_256_(h);
    float f = h - i;
    float p = hsv.z * (1.0f - hsv.y);
    float q = hsv[2] * (1.0f - hsv[1] * f);
    float t = hsv[2] * (1.0f - hsv[1] * (1.0f - f));

    switch (i) {
      case 0:
        rgb[0] = hsv[2];
        rgb[1] = t;
        rgb[2] = p;
        break;

      case 1:
        rgb[0] = q;
        rgb[1] = hsv[2];
        rgb[2] = p;
        break;

      case 2:
        rgb[0] = p;
        rgb[1] = hsv[2];
        rgb[2] = t;
        break;

      case 3:
        rgb[0] = p;
        rgb[1] = q;
        rgb[2] = hsv[2];
        break;

      case 4:
        rgb[0] = t;
        rgb[1] = p;
        rgb[2] = hsv[2];
        break;

      case 5:
        rgb[0] = hsv[2];
        rgb[1] = p;
        rgb[2] = q;
        break;

      default:
        ASSERT(!("HSVtoRGB(): invalid hue"));
        break;
    }
  }

  CImVector &CImVector::operator=(const C3Vector &c) {
    a = 255;
    r = CMath::ftol_0_256_(c.x * 255.0f);
    g = CMath::ftol_0_256_(c.y * 255.0f);
    b = CMath::ftol_0_256_(c.z * 255.0f);
    return *this;
  }

  CImVector::operator C3Vector() const {
    return C3Vector(r * 0.0039215689f, g * 0.0039215689f, b * 0.0039215689f);
  }

}  // namespace NTempest
