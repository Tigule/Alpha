#pragma once

#include "Base/Handle.h"
#include "Base/Color.h"
#include "Tempest/c33matrix.h"
#include "Tempest/c3vector.h"
#include "Tempest/cimvector.h"

#include <stddef.h>
#include <string.h>
#include <stpl.h>

DECLARE_DERIVED_HANDLE(HDATAMGR, HOBJECT);

namespace NTempest {
  bool operator==(const CImVector &left, const CImVector &right);
  bool operator!=(const CImVector &left, const CImVector &right);
  bool operator==(const C33Matrix &left, const C33Matrix &right);
  bool operator!=(const C33Matrix &left, const C33Matrix &right);
}  // namespace NTempest

struct UpdateInfo {
  void (*updateFcn)(float, LPVOID, LPVOID);
  LPVOID updateData;
  float  updatePriority;

  UpdateInfo();
};

void  DataMgrGetCoord(HDATAMGR mgr, UINT fieldId, NTempest::C3Vector *coord);
float DataMgrGetFloat(HDATAMGR mgr, UINT fieldId);

void DataMgrSetCoord(HDATAMGR mgr, UINT fieldId, const NTempest::C3Vector &coord, UINT coordFlags);

void DataMgrSetFloat(HDATAMGR mgr, UINT fieldId, float val);

void DataMgrSetBoolUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, int *), LPVOID updateData, float updatePriority);
void DataMgrSetColorUpdate(
    HDATAMGR mgr,
    UINT     fieldId,
    void (*updateFcn)(float, LPVOID, NTempest::CImVector *),
    LPVOID updateData,
    float  updatePriority
);
void DataMgrSetColorUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, C3Color *), LPVOID updateData, float updatePriority);
void DataMgrSetCoordUpdate(
    HDATAMGR mgr,
    UINT     fieldId,
    void (*updateFcn)(float, LPVOID, NTempest::C3Vector *),
    LPVOID updateData,
    float  updatePriority
);
void DataMgrSetC33MatrixUpdate(
    HDATAMGR mgr,
    UINT     fieldId,
    void (*updateFcn)(float, LPVOID, NTempest::C33Matrix *),
    LPVOID updateData,
    float  updatePriority
);
void DataMgrSetIntUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, int *), LPVOID updateData, float updatePriority);
void DataMgrSetFloatUpdate(HDATAMGR mgr, UINT fieldId, void (*updateFcn)(float, LPVOID, float *), LPVOID updateData, float updatePriority);
