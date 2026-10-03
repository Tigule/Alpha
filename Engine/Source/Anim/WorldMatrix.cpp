#include <Base/Base.h>

#include "WorldMatrix.h"

#include "Services/MatrixStack.h"
#include "Tempest/c3vector.h"
#include "Tempest/c4quaternion.h"

template <class T>
inline void CMatrixStack<T>::Mult(const T &value) {
  T &matrix = Get();
  matrix = value * matrix;
}

template <class T>
inline void CMatrixStack<T>::Remove(UINT removeFlags) {
  T &matrix = Get();

  switch (removeFlags & 6) {
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

    case 4:
      matrix.Row0AsVec3()->Set(matrix.Row0AsVec3()->Mag(), 0.0f, 0.0f);
      matrix.Row1AsVec3()->Set(0.0f, matrix.Row1AsVec3()->Mag(), 0.0f);
      matrix.Row2AsVec3()->Set(0.0f, 0.0f, matrix.Row2AsVec3()->Mag());
      break;

    case 2:
      matrix.Row0AsVec3()->Normalize();
      matrix.Row1AsVec3()->Normalize();
      matrix.Row2AsVec3()->Normalize();
      break;
  }

  if (removeFlags & 1) {
    matrix.d0 = 0.0f;
    matrix.d1 = 0.0f;
    matrix.d2 = 0.0f;
  }
}

static CMatrixStack<NTempest::C34Matrix> s_worldMatrixStack;

void WorldMatrixPush() {
  s_worldMatrixStack.Push();
}

void WorldMatrixPop() {
  s_worldMatrixStack.Pop();
}

void WorldMatrixMult(const NTempest::C34Matrix &matrix) {
  s_worldMatrixStack.Mult(matrix);
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
  NTempest::C34Matrix &matrix = s_worldMatrixStack.Get();
  NTempest::C34Matrix  rotationBasis(x, y, z);
  matrix = rotationBasis * matrix;
}

void WorldMatrixRemove(UINT removeFlags) {
  s_worldMatrixStack.Remove(removeFlags);
}

void WorldMatrixGet(NTempest::C34Matrix *m) {
  s_worldMatrixStack.Get(m);
}

void WorldMatrixTransform(NTempest::C3Vector *v) {
  *v *= s_worldMatrixStack.Get();
}

void WorldMatrixGetRow(UINT row, NTempest::C3Vector *v) {
  switch (row) {
    case 0: *v = *s_worldMatrixStack.Get().Row0AsVec3(); break;
    case 1: *v = *s_worldMatrixStack.Get().Row1AsVec3(); break;
    case 2: *v = *s_worldMatrixStack.Get().Row2AsVec3(); break;
    case 3: *v = *s_worldMatrixStack.Get().Row3AsVec3(); break;
  }
}

void WorldMatrixSetRow(UINT row, const NTempest::C3Vector &v) {
  switch (row) {
    case 0: *s_worldMatrixStack.Get().Row0AsVec3() = v; break;
    case 1: *s_worldMatrixStack.Get().Row1AsVec3() = v; break;
    case 2: *s_worldMatrixStack.Get().Row2AsVec3() = v; break;
    case 3: *s_worldMatrixStack.Get().Row3AsVec3() = v; break;
  }
}
