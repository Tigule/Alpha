#include <Base/Base.h>

#include "Tempest/caabox.h"
#include "Tempest/cdyntable.h"

namespace NTempest {

  CAaBox CAaBox::Bounding(const C3Vector *vectors, DWORD count) {
    ASSERT(vectors != 0);

    if (!count) {
      return CAaBox();
    }

    CAaBox extents(vectors[0], vectors[0]);
    for (DWORD i = 1; i < count; ++i) {
      extents.b = C3Vector::Min(extents.b, vectors[i]);
      extents.t = C3Vector::Max(extents.t, vectors[i]);
    }

    return extents;
  }

  CAaBox CAaBox::Bounding(const CDynTable<DWORD> &index, const CDynTable<C3Vector> &vects) {
    ASSERT(index.IsValid() && vects.IsValid());

    CAaBox extents;
    if (index.Used()) {
      extents.b = vects[index[0]];
      extents.t = vects[index[0]];

      for (DWORD i = 1; i < index.Used(); ++i) {
        extents.b = C3Vector::Min(extents.b, vects[index[i]]);
        extents.t.Maximize(vects[index[i]]);
      }
    }

    return extents;
  }

}  // namespace NTempest
