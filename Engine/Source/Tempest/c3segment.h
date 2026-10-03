#pragma once

#include "Tempest/c3vector.h"

namespace NTempest {

  class CAaBox;

  class C3Segment {
   public:
    C3Vector start;
    C3Vector end;

    C3Segment() {
    }

    C3Segment(const C3Vector &start, const C3Vector &end) : start(start), end(end) {
    }

    C3Vector Direction() const {
      return end - start;
    }
    C3Vector Point(float distance) const {
      return Direction() * distance + start;
    }
    CAaBox   AaBox() const;
  };

}  // namespace NTempest
