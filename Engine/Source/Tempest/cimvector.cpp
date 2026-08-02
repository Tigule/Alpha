#include <Base/Base.h>

#include "Tempest/cimvector.h"

#include "Tempest/c3vector.h"

namespace NTempest {

  void RGBtoHSV(const C3Vector &rgb, C3Vector &hsv) {
    unsigned int max = rgb.MajorAxis();
    unsigned int min = rgb.MinorAxis();

    hsv.z = rgb[max];
    if (hsv.z == 0.0f) {
      hsv.y = 0.0f;
    } else {
      hsv.y = (rgb[max] - rgb[min]) / rgb[max];
    }

    if (hsv.y == 0.0f) {
      hsv.x = -1.0f;
      return;
    }

    float delta = rgb[max] - rgb[min];
    switch (max) {
      case C3Vector::C3AXIS_X:
        hsv.x = (rgb[C3Vector::C3AXIS_Y] - rgb[C3Vector::C3AXIS_Z]) / delta;
        break;

      case C3Vector::C3AXIS_Y:
        hsv.x = 2.0f + (rgb[C3Vector::C3AXIS_Z] - rgb[C3Vector::C3AXIS_X]) / delta;
        break;

      case C3Vector::C3AXIS_Z:
        hsv.x = 4.0f + (rgb[C3Vector::C3AXIS_X] - rgb[C3Vector::C3AXIS_Y]) / delta;
        break;
    }

    hsv[C3Vector::C3AXIS_X] *= 60.0f;
    if (hsv[C3Vector::C3AXIS_X] < 0.0f) {
      hsv[C3Vector::C3AXIS_X] += 360.0f;
    }
  }

  void HSVtoRGB(const C3Vector &hsv, C3Vector &rgb) {
    if (hsv.y == 0.0f) {
      rgb.x = hsv.z;
      rgb.y = hsv.z;
      rgb.z = hsv.z;
      return;
    }

    float h = hsv.x;
    if (h == 360.0f) {
      h = 0.0f;
    }
    h /= 60.0f;

    int   i = CMath::ftol_0_256_(h);
    float f = h - i;
    float p = hsv.z * (1.0f - hsv.y);
    float q = hsv.z * (1.0f - hsv.y * f);
    float t = hsv.z * (1.0f - hsv.y * (1.0f - f));

    switch (i) {
      case 0:
        rgb[C3Vector::C3AXIS_X] = hsv[C3Vector::C3AXIS_Z];
        rgb[C3Vector::C3AXIS_Y] = t;
        rgb[C3Vector::C3AXIS_Z] = p;
        break;

      case 1:
        rgb[C3Vector::C3AXIS_X] = q;
        rgb[C3Vector::C3AXIS_Y] = hsv[C3Vector::C3AXIS_Z];
        rgb[C3Vector::C3AXIS_Z] = p;
        break;

      case 2:
        rgb[C3Vector::C3AXIS_X] = p;
        rgb[C3Vector::C3AXIS_Y] = hsv[C3Vector::C3AXIS_Z];
        rgb[C3Vector::C3AXIS_Z] = t;
        break;

      case 3:
        rgb[C3Vector::C3AXIS_X] = p;
        rgb[C3Vector::C3AXIS_Y] = q;
        rgb[C3Vector::C3AXIS_Z] = hsv[C3Vector::C3AXIS_Z];
        break;

      case 4:
        rgb[C3Vector::C3AXIS_X] = t;
        rgb[C3Vector::C3AXIS_Y] = p;
        rgb[C3Vector::C3AXIS_Z] = hsv[C3Vector::C3AXIS_Z];
        break;

      case 5:
        rgb[C3Vector::C3AXIS_X] = hsv[C3Vector::C3AXIS_Z];
        rgb[C3Vector::C3AXIS_Y] = p;
        rgb[C3Vector::C3AXIS_Z] = q;
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
