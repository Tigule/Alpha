#pragma once

#include "Tempest/c3vector.h"

namespace NTempest {

  class C3Segment {
   public:
    C3Segment() {
    }

    C3Segment(const C3Vector &start, const C3Vector &end) : start(start), end(end) {
    }

    C3Vector start;
    C3Vector end;
  };

}  // namespace NTempest
