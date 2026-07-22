#include "UIUtil/HealthBar.h"

#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

#include <Tempest/cimvector.h>

static int __fastcall SimpleHealthUpdateHandler(unsigned __int64 guid, unsigned int offset, unsigned int bytes, const void *data, void *parameter) {
  CGSimpleHealthBar *healthBar = static_cast<CGSimpleHealthBar *>(parameter);
  CGUnit_C          *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    healthBar->SetUnit(unit);
  }
  return 1;
}

CGSimpleHealthBar::CGSimpleHealthBar(CSimpleFrame *parent) : CSimpleStatusBar(parent), m_unitGUID(0), m_scaleColor(1) {
}

CGSimpleHealthBar::~CGSimpleHealthBar() {
  RemoveMirrorHandlers();
}

void CGSimpleHealthBar::SetUnit(CGUnit_C *unit) {
  RemoveMirrorHandlers();
  if (!unit) {
    m_unitGUID = 0;
    return;
  }

  m_unitGUID = unit->GetGUID();
  const CGUnitData *unitData = unit->GetUnitData();
  SetMinMaxValues(0.0f, static_cast<float>(unitData->maxHealth));
  SetValue(static_cast<float>(unitData->health));
  InstallMirrorHandlers();
}

void CGSimpleHealthBar::SetValue(float value) {
  CSimpleStatusBar::SetValue(value);
  if (m_scaleColor) {
    float range = GetMaxValue() - GetMinValue();
    float scale = range > 0.0f ? (value - GetMinValue()) / range : 0.0f;
    if (scale < 0.0f) {
      scale = 0.0f;
    } else if (scale > 1.0f) {
      scale = 1.0f;
    }
    NTempest::CImVector color;
    color.Set(1.0f - scale, scale, 0.0f, 1.0f);
    CSimpleStatusBar::SetStatusBarColor(color);
  }
}

void CGSimpleHealthBar::SetStatusBarColor(const NTempest::CImVector &color) {
  m_scaleColor = 0;
  CSimpleStatusBar::SetStatusBarColor(color);
}

void CGSimpleHealthBar::InstallMirrorHandlers() {
  if (!m_unitGUID) {
    return;
  }
  ClntObjMgrSetObjMirrorHandler(m_unitGUID, CGUnit_C::OffsetOf(ID_UNIT) + 64, 4, SimpleHealthUpdateHandler, this, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(m_unitGUID, CGUnit_C::OffsetOf(ID_UNIT) + 84, 4, SimpleHealthUpdateHandler, this, HANDLER_PRIORITY_NORMAL);
}

void CGSimpleHealthBar::RemoveMirrorHandlers() {
  if (!m_unitGUID) {
    return;
  }
  ClntObjMgrUnsetObjMirrorHandler(m_unitGUID, CGUnit_C::OffsetOf(ID_UNIT) + 64, SimpleHealthUpdateHandler, this);
  ClntObjMgrUnsetObjMirrorHandler(m_unitGUID, CGUnit_C::OffsetOf(ID_UNIT) + 84, SimpleHealthUpdateHandler, this);
}
