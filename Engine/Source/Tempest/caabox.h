#ifndef ENGINE_SOURCE_TEMPEST_CAABOX_H
#define ENGINE_SOURCE_TEMPEST_CAABOX_H

#include "Tempest/c3vector.h"

namespace NTempest {

  class CAaBox {
   public:
    CAaBox(float value = 0.0f) : b(value), t(value) {
    }
    CAaBox(const C3Vector &value) : b(value), t(value) {
    }
    CAaBox(const C3Vector &bottom, const C3Vector &top) : b(bottom), t(top) {
    }
    ~CAaBox() {
    }

    static CAaBox __fastcall Bounding(const C3Vector *vectors, unsigned long count);

    C3Vector b;
    C3Vector t;
  };

}  // namespace NTempest

#endif
