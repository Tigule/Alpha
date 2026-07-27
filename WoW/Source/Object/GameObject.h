#ifndef WOW_SOURCE_OBJECT_GAMEOBJECT_H
#define WOW_SOURCE_OBJECT_GAMEOBJECT_H

#include "Tempest/c44matrix.h"

namespace NTempest {
class C3Vector;
}

void GenerateChairPoints(
    const NTempest::C44Matrix &matrix,
    unsigned int slots,
    NTempest::C3Vector *out);

#endif
