#include "CGxDeviceOpenGl.h"

void CGxDeviceOpenGl::CapsWindowSize(NTempest::CRect &dst) {
  dst = DeviceCurWindow();
}

int CGxDeviceOpenGl::CapsIsWindowVisible() {
  const NTempest::CRect &rect = DeviceCurWindow();
  return (rect.r - rect.l) * (rect.b - rect.t) > 1.0f;
}
