#pragma once

#include "Tempest/c34matrix.h"

void WorldMatrixPush();
void WorldMatrixPop();
void WorldMatrixMult(const NTempest::C34Matrix &matrix);
void WorldMatrixLoad(const NTempest::C34Matrix &matrix);
void WorldMatrixLoadIdentity();
void WorldMatrixTranslate(const NTempest::C3Vector &move);
void WorldMatrixRotate(const NTempest::C4Quaternion &rotation);
void WorldMatrixRotate(float angle, const NTempest::C3Vector &axis);
void WorldMatrixScale(const NTempest::C3Vector &scale);
void WorldMatrixScale(float scale);
void WorldMatrixBasis(const NTempest::C3Vector &x, const NTempest::C3Vector &y, const NTempest::C3Vector &z);
void WorldMatrixRemove(unsigned int removeFlags);
void WorldMatrixGet(NTempest::C34Matrix *m);
void WorldMatrixTransform(NTempest::C3Vector *v);
void WorldMatrixGetRow(unsigned int row, NTempest::C3Vector *v);
void WorldMatrixSetRow(unsigned int row, const NTempest::C3Vector &v);
