#pragma once

#include "Tempest/c34matrix.h"

void __fastcall WorldMatrixPush();
void __fastcall WorldMatrixPop();
void __fastcall WorldMatrixMult(const NTempest::C34Matrix &matrix);
void __fastcall WorldMatrixLoad(const NTempest::C34Matrix &matrix);
void __fastcall WorldMatrixLoadIdentity();
void __fastcall WorldMatrixTranslate(const NTempest::C3Vector &move);
void __fastcall WorldMatrixRotate(const NTempest::C4Quaternion &rotation);
void __fastcall WorldMatrixRotate(float angle, const NTempest::C3Vector &axis);
void __fastcall WorldMatrixScale(const NTempest::C3Vector &scale);
void __fastcall WorldMatrixScale(float scale);
void __fastcall WorldMatrixBasis(const NTempest::C3Vector &x, const NTempest::C3Vector &y, const NTempest::C3Vector &z);
void __fastcall WorldMatrixRemove(unsigned int removeFlags);
void __fastcall WorldMatrixGet(NTempest::C34Matrix *m);
void __fastcall WorldMatrixTransform(NTempest::C3Vector *v);
void __fastcall WorldMatrixGetRow(unsigned int row, NTempest::C3Vector *v);
void __fastcall WorldMatrixSetRow(unsigned int row, const NTempest::C3Vector &v);
