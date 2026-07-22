#include "Ui/NamePlateFrame.h"

#include "Object/ObjectClient/Unit_C.h"
#include "UIUtil/HealthBar.h"
#include "Ui/GameUI.h"

#include <Frame/CSimpleRender.h>
#include <storm.h>

CGNamePlateFrame::CGNamePlateFrame(CSimpleFrame *parent) : CSimpleButton(parent), m_unit(0), m_highlight(0), m_nameFrame(0), m_healthBar(0) {
  m_highlight = NEW(CSimpleTexture)(this, 2, 1);
  m_highlight->SetTexture("Interface\\Tooltips\\Nameplate-Border", 0);
  m_highlight->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
  m_highlight->SetPoint(FRAMEPOINT_BOTTOMRIGHT, this, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f, 1);
  m_highlight->Hide();

  m_nameFrame = NEW(CSimpleFontString)(this, 2, 1);
  m_nameFrame->SetPoint(FRAMEPOINT_BOTTOM, this, FRAMEPOINT_TOP, 0.0f, 0.005f, 1);
  m_nameFrame->SetFont("Fonts\\FRIZQT__.TTF", 0.011f, 0);

  m_healthBar = NEW(CGSimpleHealthBar)(this);
  m_healthBar->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, 0.006f, -0.006f, 1);
  m_healthBar->SetPoint(FRAMEPOINT_BOTTOMRIGHT, this, FRAMEPOINT_BOTTOMRIGHT, -0.006f, 0.006f, 1);
  SetWidth(0.11f);
  SetHeight(0.025f);
  SetClickAction(MOUSE_BUTTON_LEFT);
}

CGNamePlateFrame::~CGNamePlateFrame() {
  DEL(m_healthBar);
  DEL(m_nameFrame);
  DEL(m_highlight);
}

void CGNamePlateFrame::Initialize(CGUnit_C *unit) {
  FATALASSERT(unit);
  m_unit = unit->GetGUID();

  char level[32];
  char buf[32];
  SStrCopy(level, "", sizeof(level));
  SStrPrintf(buf, sizeof(buf), "%s %d", level, unit->GetUnitData()->level);
  m_nameFrame->SetText(unit->GetUnitName());
  m_healthBar->SetUnit(unit);

  NTempest::CImVector color;
  UNIT_REACTION       reaction = unit->UnitReaction(0);
  if (reaction <= UNIT_REACTION_HOSTILE) {
    color.Set(1.0f, 0.0f, 0.0f, 1.0f);
  } else if (reaction <= UNIT_REACTION_NEUTRAL) {
    color.Set(1.0f, 1.0f, 0.0f, 1.0f);
  } else {
    color.Set(0.0f, 1.0f, 0.0f, 1.0f);
  }
  m_healthBar->SetStatusBarColor(color);

  float width = m_nameFrame->GetStringWidth();
  if (width < m_healthBar->GetWidth()) {
    width = m_healthBar->GetWidth();
  }
  SetWidth(width + 0.01f);
  SetHeight(m_nameFrame->GetStringHeight() + m_healthBar->GetHeight() + 0.016f);
}

void CGNamePlateFrame::OnLayerCursorEnter() {
  CSimpleButton::OnLayerCursorEnter();
  m_highlight->Show();
}

void CGNamePlateFrame::OnLayerCursorExit() {
  CSimpleButton::OnLayerCursorExit();
  m_highlight->Hide();
}

void CGNamePlateFrame::OnClick(MOUSEBUTTON button) {
  CGGameUI::Target(m_unit, 0);
  CSimpleButton::OnClick(button);
}
