#pragma once

#include "Tempest/c33matrix.h"

namespace NTempest {

  class CObBox {
   public:
    CObBox() {
    }

    CObBox(const C3Vector &center, const C3Vector &extent)
        : c(center), e(extent) {
    }

    CObBox(const C3Vector &center, const C3Vector &extent, C33Matrix &basis)
        : c(center), e(extent), b(basis) {
    }

    ~CObBox();

    C3Vector c;
    C3Vector e;
    C33Matrix b;
  };

}  // namespace NTempest
