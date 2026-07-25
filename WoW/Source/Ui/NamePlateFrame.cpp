#include "Ui/NamePlateFrame.h"

#include "Object/ObjectClient/Unit_C.h"
#include "UIUtil/HealthBar.h"
#include "Ui/GameUI.h"

#include <Frame/CBackdropGenerator.h>
#include <Frame/CSimpleRender.h>
#include <FrameScript/FrameScript.h>
#include <storm.h>

CGNamePlateFrame::CGNamePlateFrame(CSimpleFrame *parent) : CSimpleButton(parent), m_unit(0), m_highlight(0), m_nameFrame(0), m_healthBar(0) {
  CBackdropGenerator *backdrop = NEW(CBackdropGenerator);
  backdrop->m_background = "Interface\\Tooltips\\UI-Tooltip-Background";
  backdrop->m_pieces = 255;
  backdrop->m_tileBackground = 1;
  backdrop->m_border = "Interface\\Tooltips\\UI-Tooltip-Border";
  backdrop->m_cornerSize = 0.01f;
  backdrop->m_leftInset = 0.0025f;
  backdrop->m_rightInset = 0.0025f;
  backdrop->m_topInset = 0.0025f;
  backdrop->m_bottomInset = 0.0025f;
  backdrop->SetVertexColor(NTempest::CImVector(0xFF161616));
  backdrop->SetBorderVertexColor(NTempest::CImVector(0xFFFFFFFF));
  SetBackdrop(backdrop);

  m_highlight = NEW(CSimpleTexture)(this, 2, 1);
  m_highlight->SetAllPoints(this, 1);
  m_highlight->SetTexture(NTempest::CImVector(0x80808080));
  m_highlight->Hide();

  m_nameFrame = NEW(CSimpleFontString)(this, 2, 1);
  m_nameFrame->SetPoint(FRAMEPOINT_TOP, this, FRAMEPOINT_TOP, 0.0f, -0.005f, 1);
  m_nameFrame->SetFont(FrameScript_GetText("NAMEPLATE_FONT", -1, GENDER_NOT_APPLICABLE), 0.0235f, 0);
  m_nameFrame->SetVertexColor(NTempest::CImVector(0xFFFFFFFF));
  m_nameFrame->AddShadow(NTempest::CImVector(0xFF0000FF), NTempest::C2Vector(0.001f, -0.001f));

  m_healthBar = NEW(CGSimpleHealthBar)(this);
  m_healthBar->SetWidth(0.08f);
  m_healthBar->SetHeight(0.005f);
  m_healthBar->SetPoint(FRAMEPOINT_TOP, m_nameFrame, FRAMEPOINT_BOTTOM, 0.0f, -0.001f, 1);
  m_healthBar->SetBarTexture("Interface\\TargetingFrame\\UI-TargetingFrame-BarFill", 2);
  SetClickAction(0x500);
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
