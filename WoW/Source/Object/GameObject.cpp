#include <Base/Base.h>
#include <Gx/Gx.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "GameObject.h"

#include "Tempest/c3vector.h"

#define MAX_CHAIR_SLOTS 5

void GenerateChairPoints(const NTempest::C44Matrix &matrix, UINT slots, NTempest::C3Vector *out) {
  FATALASSERT(out);
  FATALASSERT(slots);
  FATALASSERT(slots <= MAX_CHAIR_SLOTS);

  NTempest::C3Vector currentSitPoint(0.0f, -(static_cast<float>(slots) - 1.0f) * 0.5f, 0.0f);
  const float        sitPointOffset = 1.0f;
  for (UINT i = 0; i < slots; ++i) {
    out[i] = currentSitPoint * matrix;
    currentSitPoint.y += sitPointOffset;
  }
}
