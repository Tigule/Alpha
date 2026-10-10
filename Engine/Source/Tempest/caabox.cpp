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
      extents.Enclose(vectors[i]);
    }

    return extents;
  }

  CAaBox CAaBox::Bounding(const CDynTable<DWORD> &index, const CDynTable<C3Vector> &vects) {
    ASSERT(index.IsValid() && vects.IsValid());

    if (!index.Used()) {
      return CAaBox();
    }

    CAaBox extents(vects[index[0]], vects[index[0]]);
    for (DWORD i = 1; i < index.Used(); ++i) {
      extents.Enclose(vects[index[i]]);
    }

    return extents;
  }

}  // namespace NTempest
