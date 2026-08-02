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
  NTempest::C34Matrix &world = s_worldMatrixStack.Get();
  world = matrix * world;
}

void WorldMatrixLoad(const NTempest::C34Matrix &matrix) {
  s_worldMatrixStack.Load(matrix);
}

void WorldMatrixLoadIdentity() {
  s_worldMatrixStack.Load(NTempest::C34Matrix());
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
  NTempest::C34Matrix &world = s_worldMatrixStack.Get();
  world = rotationBasis * world;
}

void WorldMatrixRemove(unsigned int removeFlags) {
  NTempest::C34Matrix &matrix = s_worldMatrixStack.Get();

  switch (removeFlags & 6) {
    case 2: {
      NTempest::C3Vector x(matrix.a0, matrix.a1, matrix.a2);
      NTempest::C3Vector y(matrix.b0, matrix.b1, matrix.b2);
      NTempest::C3Vector z(matrix.c0, matrix.c1, matrix.c2);

      x.Normalize();
      y.Normalize();
      z.Normalize();

      matrix.a0 = x.x;
      matrix.a1 = x.y;
      matrix.a2 = x.z;
      matrix.b0 = y.x;
      matrix.b1 = y.y;
      matrix.b2 = y.z;
      matrix.c0 = z.x;
      matrix.c1 = z.y;
      matrix.c2 = z.z;
      break;
    }

    case 4: {
      NTempest::C3Vector x(matrix.a0, matrix.a1, matrix.a2);
      NTempest::C3Vector y(matrix.b0, matrix.b1, matrix.b2);
      NTempest::C3Vector z(matrix.c0, matrix.c1, matrix.c2);

      matrix.a0 = x.Mag();
      matrix.a1 = 0.0f;
      matrix.a2 = 0.0f;
      matrix.b0 = 0.0f;
      matrix.b1 = y.Mag();
      matrix.b2 = 0.0f;
      matrix.c0 = 0.0f;
      matrix.c1 = 0.0f;
      matrix.c2 = z.Mag();
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

void WorldMatrixGetRow(unsigned int row, NTempest::C3Vector *v) {
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

void WorldMatrixSetRow(unsigned int row, const NTempest::C3Vector &v) {
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
