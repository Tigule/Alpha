#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "GameObject.h"

#include "Tempest/c3vector.h"

void GenerateChairPoints(const NTempest::C44Matrix &matrix, UINT slots, NTempest::C3Vector *out) {
  FATALASSERT(out);
  FATALASSERT(slots);
  FATALASSERT(slots <= 5);

  NTempest::C3Vector currentSitPoint(0.0f, -(static_cast<float>(slots) - 1.0f) * 0.5f, 0.0f);
  const float        sitPointOffset = 1.0f;
  for (UINT i = 0; i < slots; ++i) {
    out[i] = currentSitPoint * matrix;
    currentSitPoint.y += sitPointOffset;
  }
}
