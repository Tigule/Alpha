#include "../Base/Base.h"

#include "Tempest/c22matrix.h"
#include "Tempest/c2ivector.h"
#include "Tempest/c2vector.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c34matrix.h"
#include "Tempest/c3ivector.h"
#include "Tempest/c3ray.h"
#include "Tempest/c3segment.h"
#include "Tempest/c3spline.h"
#include "Tempest/c3vector.h"
#include "Tempest/c44matrix.h"
#include "Tempest/c4ivector.h"
#include "Tempest/c4plane.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/c4quaternioncompressed.h"
#include "Tempest/c4vector.h"
#include "Tempest/caabox.h"
#include "Tempest/caasphere.h"
#include "Tempest/ccone.h"
#include "Tempest/cdyntable.h"
#include "Tempest/cfacet.h"
#include "Tempest/cimvector.h"
#include "Tempest/cirange.h"
#include "Tempest/cirect.h"
#include "Tempest/cmath.h"
#include "Tempest/cmemblock.h"
#include "Tempest/cobbox.h"
#include "Tempest/cpriorityq.h"
#include "Tempest/crandom.h"
#include "Tempest/crange.h"
#include "Tempest/crect.h"
#include "Tempest/tempest_intersect.h"

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
