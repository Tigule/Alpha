#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include "UIUtil/Camera.h"
#include "UIUtil/InputControl.h"
#include "UIUtil/Tooltip.h"
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Ui/GameUI.h"

#include "UIUtil/HealthBar.h"

#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

#include <Frame/CSimpleRender.h>
#include <Tempest/cimvector.h>

static BOOL SimpleHealthUpdateHandler(DWORDLONG guid, UINT, UINT, LPCVOID data, LPVOID parameter) {
  FATALASSERT(parameter);

  CGSimpleHealthBar *healthBar = (CGSimpleHealthBar *)parameter;
  CGUnit_C          *unit = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
  if (unit) {
    healthBar->SetMinMaxValues(0.0f, unit->GetMaxHealth());
    healthBar->SetValue(unit->GetHealth());
  }

  return 1;
}

CGSimpleHealthBar::CGSimpleHealthBar(CSimpleFrame *parent) : CSimpleStatusBar(parent), m_unitGUID(0) {
  m_scaleColor = 1;
}

CGSimpleHealthBar::~CGSimpleHealthBar() {
  SetUnit(0);
}

void CGSimpleHealthBar::SetUnit(CGUnit_C *unit) {
  RemoveMirrorHandlers();
  if (!unit) {
    m_unitGUID = 0;
    return;
  }

  m_unitGUID = unit->GetGUID();
  InstallMirrorHandlers();

  SetMinMaxValues(0.0f, unit->GetMaxHealth());
  SetValue(unit->GetHealth());
}

void CGSimpleHealthBar::SetValue(float value) {
  CSimpleStatusBar::SetValue(value);
  if (m_scaleColor) {
    NTempest::CImVector color(GetAnimValue() <= 0.2f ? 0xFFFF0000 : 0xFF00FF00);
    SetStatusBarColor(color);
    m_scaleColor = 1;
  }
}

void CGSimpleHealthBar::SetStatusBarColor(const NTempest::CImVector &color) {
  if (m_barTexture) {
    m_barTexture->SetVertexColor(color);
  }
  m_scaleColor = 0;
}

void CGSimpleHealthBar::InstallMirrorHandlers() {
  if (!m_unitGUID) {
    return;
  }
  ClntObjMgrSetObjMirrorHandler(m_unitGUID, CGUnit_C::OffsetOf(ID_UNIT) + offsetof(CGUnitData, health), sizeof(((CGUnitData *)0)->health), SimpleHealthUpdateHandler, this, HANDLER_PRIORITY_NORMAL);
  ClntObjMgrSetObjMirrorHandler(m_unitGUID, CGUnit_C::OffsetOf(ID_UNIT) + offsetof(CGUnitData, maxHealth), sizeof(((CGUnitData *)0)->maxHealth), SimpleHealthUpdateHandler, this, HANDLER_PRIORITY_NORMAL);
}

void CGSimpleHealthBar::RemoveMirrorHandlers() {
  if (!m_unitGUID) {
    return;
  }
  ClntObjMgrUnsetObjMirrorHandler(m_unitGUID, CGUnit_C::OffsetOf(ID_UNIT) + offsetof(CGUnitData, health), SimpleHealthUpdateHandler, this);
  ClntObjMgrUnsetObjMirrorHandler(m_unitGUID, CGUnit_C::OffsetOf(ID_UNIT) + offsetof(CGUnitData, maxHealth), SimpleHealthUpdateHandler, this);
}
