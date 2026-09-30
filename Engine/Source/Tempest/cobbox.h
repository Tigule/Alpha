#pragma once

#include "Tempest/c33matrix.h"

namespace NTempest {

  class CObBox {
   public:
    C3Vector  c;
    C3Vector  e;
    C33Matrix b;

    CObBox() {
    }

    CObBox(const C3Vector &center, const C3Vector &extent, C33Matrix &basis) : c(center), e(extent), b(basis) {
    }

    CObBox(const C3Vector &center, const C3Vector &extent) : c(center), e(extent) {
    }

    ~CObBox();
  };

}  // namespace NTempest
