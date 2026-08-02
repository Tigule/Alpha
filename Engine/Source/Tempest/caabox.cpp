#include <Base/Base.h>

#include "Tempest/caabox.h"
#include "Tempest/cdyntable.h"

namespace NTempest {

  CAaBox CAaBox::Bounding(const C3Vector *vectors, unsigned long count) {
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

  CAaBox CAaBox::Bounding(const CDynTable<unsigned long> &index, const CDynTable<C3Vector> &vects) {
    ASSERT(index.IsValid() && vects.IsValid());

    CAaBox extents;
    if (index.Used()) {
      extents.b = vects[index[0]];
      extents.t = vects[index[0]];

      for (unsigned long i = 1; i < index.Used(); ++i) {
        extents.b = C3Vector::Min(extents.b, vects[index[i]]);
        extents.t.Maximize(vects[index[i]]);
      }
    }

    return extents;
  }

}  // namespace NTempest
