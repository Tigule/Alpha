#include <Base/Base.h>

#include "WorldMatrix.h"

#include "Services/MatrixStack.h"
#include "Tempest/c3vector.h"
#include "Tempest/c4quaternion.h"

static CMatrixStack<NTempest::C34Matrix> s_worldMatrixStack;

void WorldMatrixPush() {
  s_worldMatrixStack.Push();
}

void WorldMatrixPop() {
  s_worldMatrixStack.Pop();
}

void WorldMatrixMult(const NTempest::C34Matrix &matrix) {
  s_worldMatrixStack.Get() = matrix * s_worldMatrixStack.Get();
}

void WorldMatrixLoad(const NTempest::C34Matrix &matrix) {
  s_worldMatrixStack.Load(matrix);
}

void WorldMatrixLoadIdentity() {
  s_worldMatrixStack.Get().Identity();
}

void WorldMatrixTranslate(const NTempest::C3Vector &move) {
  s_worldMatrixStack.Get().Translate(move);
}

void WorldMatrixRotate(const NTempest::C4Quaternion &rotation) {
  s_worldMatrixStack.Get().Rotate(rotation);
}

void WorldMatrixRotate(float angle, const NTempest::C3Vector &axis) {
  s_worldMatrixStack.Get().Rotate(angle, axis, true);
}

void WorldMatrixScale(const NTempest::C3Vector &scale) {
  s_worldMatrixStack.Get().Scale(scale);
}

void WorldMatrixScale(float scale) {
  s_worldMatrixStack.Get().Scale(scale);
}

void WorldMatrixBasis(const NTempest::C3Vector &x, const NTempest::C3Vector &y, const NTempest::C3Vector &z) {
  NTempest::C34Matrix  rotationBasis(x, y, z);
  s_worldMatrixStack.Get() = rotationBasis * s_worldMatrixStack.Get();
}

void WorldMatrixRemove(UINT removeFlags) {
  NTempest::C34Matrix &matrix = s_worldMatrixStack.Get();

  switch (removeFlags & 6) {
    case 2: {
      matrix.Row0AsVec3()->Normalize();
      matrix.Row1AsVec3()->Normalize();
      matrix.Row2AsVec3()->Normalize();
      break;
    }

    case 4: {
      matrix.a0 = matrix.Row0AsVec3()->Mag();
      matrix.a1 = 0.0f;
      matrix.a2 = 0.0f;
      matrix.b1 = matrix.Row1AsVec3()->Mag();
      matrix.b0 = 0.0f;
      matrix.b2 = 0.0f;
      matrix.c2 = matrix.Row2AsVec3()->Mag();
      matrix.c0 = 0.0f;
      matrix.c1 = 0.0f;
      break;
    }

    case 6:
      matrix.a0 = 1.0f;
      matrix.a1 = 0.0f;
      matrix.a2 = 0.0f;
      matrix.b0 = 0.0f;
      matrix.b1 = 1.0f;
      matrix.b2 = 0.0f;
      matrix.c0 = 0.0f;
      matrix.c1 = 0.0f;
      matrix.c2 = 1.0f;
      break;
  }

  if (removeFlags & 1) {
    matrix.d0 = 0.0f;
    matrix.d1 = 0.0f;
    matrix.d2 = 0.0f;
  }
}

void WorldMatrixGet(NTempest::C34Matrix *m) {
  s_worldMatrixStack.Get(m);
}

void WorldMatrixTransform(NTempest::C3Vector *v) {
  *v *= s_worldMatrixStack.Get();
}

void WorldMatrixGetRow(UINT row, NTempest::C3Vector *v) {
  switch (row) {
    case 0: {
      NTempest::C34Matrix &matrix = s_worldMatrixStack.Get();
      v->x = matrix.a0;
      v->y = matrix.a1;
      v->z = matrix.a2;
      break;
    }

    case 1: {
      NTempest::C34Matrix &matrix = s_worldMatrixStack.Get();
      v->x = matrix.b0;
      v->y = matrix.b1;
      v->z = matrix.b2;
      break;
    }

    case 2: {
      NTempest::C34Matrix &matrix = s_worldMatrixStack.Get();
      v->x = matrix.c0;
      v->y = matrix.c1;
      v->z = matrix.c2;
      break;
    }

    case 3: {
      NTempest::C34Matrix &matrix = s_worldMatrixStack.Get();
      v->x = matrix.d0;
      v->y = matrix.d1;
      v->z = matrix.d2;
      break;
    }
  }
}

void WorldMatrixSetRow(UINT row, const NTempest::C3Vector &v) {
  switch (row) {
    case 0: {
      NTempest::C34Matrix &matrix = s_worldMatrixStack.Get();
      matrix.a0 = v.x;
      matrix.a1 = v.y;
      matrix.a2 = v.z;
      break;
    }

    case 1: {
      NTempest::C34Matrix &matrix = s_worldMatrixStack.Get();
      matrix.b0 = v.x;
      matrix.b1 = v.y;
      matrix.b2 = v.z;
      break;
    }

    case 2: {
      NTempest::C34Matrix &matrix = s_worldMatrixStack.Get();
      matrix.c0 = v.x;
      matrix.c1 = v.y;
      matrix.c2 = v.z;
      break;
    }

    case 3: {
      NTempest::C34Matrix &matrix = s_worldMatrixStack.Get();
      matrix.d0 = v.x;
      matrix.d1 = v.y;
      matrix.d2 = v.z;
      break;
    }
  }
}
