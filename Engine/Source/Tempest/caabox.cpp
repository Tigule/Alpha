#include "Tempest/caabox.h"

namespace NTempest {

  CAaBox __fastcall CAaBox::Bounding(const C3Vector *vectors, unsigned long count) {
    ASSERT(vectors != 0);

    CAaBox extents;
    if (count) {
      extents.b = vectors[0];
      extents.t = vectors[0];

      for (unsigned long i = 1; i < count; ++i) {
        extents.b = C3Vector(
            vectors[i].x <= extents.b.x ? vectors[i].x : extents.b.x, vectors[i].y <= extents.b.y ? vectors[i].y : extents.b.y,
            vectors[i].z <= extents.b.z ? vectors[i].z : extents.b.z
        );
        extents.t.Maximize(vectors[i]);
      }
    }

    return extents;
  }

}  // namespace NTempest
