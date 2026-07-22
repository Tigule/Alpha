#include "Frame/CSimpleButton.h"

#include "Event/CMouseEvent.h"
#include "Event/CObserver.h"
#include "Frame/CSimpleRender.h"
#include "Frame/CSimpleTop.h"
#include "FrameXML/LoadXML.h"
#include "FrameXML/XMLTree.h"

#include <limits.h>
#include <string.h>

class CSimpleButtonClickEvent : public CEvent {
 public:
  CSimpleButtonClickEvent(unsigned int id) : CEvent(id) {
  }

  MOUSEBUTTON button;
};

static CEvent s_trackEvent;

CSimpleButton::CSimpleButton(CSimpleFrame *parent)
    : CSimpleFrame(parent),
      m_observer(0),
      m_trackObserver(0),
      m_state(BUTTONSTATE_DISABLED),
      m_stateLocked(0),
      m_clickAction(0x100),
      m_disabledText(0),
      m_text(0),
      m_highlightText(0),
      m_activeTexture(0),
      m_onClick(0) {
  m_pressedOffset.x = 0.001f;
  m_pressedOffset.y = -0.001f;

  memset(m_textures, 0, sizeof(m_textures));

  Enable(1);
  EnableEvent(SIMPLE_EVENT_MOUSE, UINT_MAX);
}

CSimpleButton::~CSimpleButton() {
  SetText(0);
  SetDisabledText(0);
  SetHighlightText(0);

  for (CSimpleButtonState state = BUTTONSTATE_DISABLED; state < NUM_BUTTONSTATES; state = static_cast<CSimpleButtonState>(state + 1)) {
    SetStateTexture(state, static_cast<CSimpleTexture *>(0));
  }

  char description[1024];
  SStrPrintf(description, sizeof(description), "%s:OnClick", GetName());
  SetEventScript(m_onClick, 0, description);
}

void CSimpleButton::LoadXML(const XMLNode *node, CStatus *status) {
  float x;
  float y;

  CSimpleFrame::LoadXML(node, status);

  for (const XMLNode *child = node->GetChild(); child; child = child->GetSibling()) {
    const char *name = child->GetName();

    if (!SStrCmpI(name, "NormalTexture", INT_MAX)) {
      SetStateTexture(BUTTONSTATE_NORMAL, LoadXML_Texture(child, this, status));
    } else if (!SStrCmpI(name, "PushedTexture", INT_MAX)) {
      SetStateTexture(BUTTONSTATE_PUSHED, LoadXML_Texture(child, this, status));
    } else if (!SStrCmpI(name, "DisabledTexture", INT_MAX)) {
      SetStateTexture(BUTTONSTATE_DISABLED, LoadXML_Texture(child, this, status));
    } else if (!SStrCmpI(name, "HighlightTexture", INT_MAX)) {
      CSimpleTexture *texture = LoadXML_Texture(child, this, status);
      SetHighlight(texture, texture->m_alphamode);
    } else if (!SStrCmpI(name, "NormalText", INT_MAX)) {
      SetText(LoadXML_String(child, this, status));
    } else if (!SStrCmpI(name, "HighlightText", INT_MAX)) {
      SetHighlightText(LoadXML_String(child, this, status));
    } else if (!SStrCmpI(name, "DisabledText", INT_MAX)) {
      SetDisabledText(LoadXML_String(child, this, status));
    } else if (!SStrCmpI(name, "PushedTextOffset", INT_MAX)) {
      if (LoadXML_Dimensions(child, x, y, status)) {
        SetPressedOffset(NTempest::C2Vector(x, y));
      }
    }
  }
}

void CSimpleButton::LoadXML_Scripts(const XMLNode *node, CStatus *status) {
  CSimpleFrame::LoadXML_Scripts(node, status);

  for (const XMLNode *script = node->GetChild(); script; script = script->GetSibling()) {
    if (!SStrCmpI(script->GetName(), "OnClick", INT_MAX)) {
      char description[1024];
      SStrPrintf(description, sizeof(description), "%s:OnClick", GetName());
      SetEventScript(m_onClick, script->GetBody(), description);
    }
  }
}

void CSimpleButton::SetText(CSimpleFontString *text) {
  if (m_text) {
    DEL(m_text);
  }

  if (text) {
    text->SetFrame(this, 2, 0);
  }

  m_text = text;
  UpdateTextState(m_state);
}

void CSimpleButton::SetDisabledText(CSimpleFontString *text) {
  if (m_disabledText) {
    DEL(m_disabledText);
  }

  if (text) {
    text->SetFrame(this, 2, 0);
  }

  m_disabledText = text;
  UpdateTextState(m_state);
}

void CSimpleButton::SetHighlightText(CSimpleFontString *text) {
  if (m_highlightText) {
    DEL(m_highlightText);
  }

  if (text) {
    text->SetFrame(this, 4, 0);
  }

  m_highlightText = text;
  UpdateTextState(m_state);
}

void CSimpleButton::SetTextString(const char *text) {
  if (m_text && text) {
    m_text->SetText(text);
  }
}

void CSimpleButton::SetDisabledTextString(const char *text) {
  if (m_disabledText && text) {
    m_disabledText->SetText(text);
  }
}

void CSimpleButton::SetHighlightTextString(const char *text) {
  if (m_highlightText && text) {
    m_highlightText->SetText(text);
  }
}

void CSimpleButton::SetTextColor(const NTempest::CImVector &color) {
  if (m_text) {
    m_text->SetVertexColor(color);
  }
}

void CSimpleButton::SetDisabledTextColor(const NTempest::CImVector &color) {
  if (m_disabledText) {
    m_disabledText->SetVertexColor(color);
  }
}

void CSimpleButton::SetHighlightTextColor(const NTempest::CImVector &color) {
  if (m_highlightText) {
    m_highlightText->SetVertexColor(color);
  }
}

void CSimpleButton::SetPressedOffset(const NTempest::C2Vector &offset) {
  m_pressedOffset = offset;
}

int CSimpleButton::SetStateTexture(CSimpleButtonState state, const char *texFile) {
  if (m_textures[state]) {
    m_textures[state]->SetTexture(texFile, 0);
    return 1;
  }

  CSimpleTexture *texture = NEW(CSimpleTexture)(0, 2, 1);
  if (texture->SetTexture(texFile, 0)) {
    texture->SetAllPoints(this, 1);
    SetStateTexture(state, texture);
    return 1;
  }

  DEL(texture);
  return 0;
}

void CSimpleButton::SetStateTexture(CSimpleButtonState state, CSimpleTexture *texture) {
  ASSERT(state < NUM_BUTTONSTATES);

  if (m_textures[state]) {
    DEL(m_textures[state]);
  }

  if (texture) {
    texture->SetFrame(this, 2, 0);
  }

  m_textures[state] = texture;
  if (texture && state == m_state) {
    m_activeTexture = texture;
    texture->Show();
  }
}

void CSimpleButton::Enable(int enabled) {
  if (enabled) {
    if (m_state == BUTTONSTATE_DISABLED) {
      SetButtonState(BUTTONSTATE_NORMAL, 0);
    }
  } else if (m_state != BUTTONSTATE_DISABLED) {
    DisableDrawLayer(4);
    SetButtonState(BUTTONSTATE_DISABLED, 0);
  }
}

void CSimpleButton::OnLayerHide() {
  if (m_state != BUTTONSTATE_DISABLED && !m_stateLocked) {
    SetButtonState(BUTTONSTATE_NORMAL, 0);
  }

  CSimpleFrame::OnLayerHide();
}

int CSimpleButton::OnLayerMouseDown(CMouseEvent &evt) {
  int handled = CSimpleFrame::OnLayerMouseDown(evt);
  if (!handled && m_state != BUTTONSTATE_DISABLED && IsMouseButtonHandled(evt.button)) {
    NTempest::C2Vector pt(evt.x, evt.y);
    if (TestHitRect(pt)) {
      handled = 1;
      if (evt.button & m_clickAction) {
        OnClick(evt.button);
      }
      if (!m_stateLocked && m_state != BUTTONSTATE_DISABLED) {
        SetButtonState(BUTTONSTATE_PUSHED, 0);
      }
    }
  }
  return handled;
}

int CSimpleButton::OnLayerMouseUp(CMouseEvent &evt) {
  int handled = CSimpleFrame::OnLayerMouseUp(evt);
  if (!handled && m_state != BUTTONSTATE_DISABLED) {
    handled = m_state == BUTTONSTATE_PUSHED;
    if (m_state == BUTTONSTATE_PUSHED) {
      NTempest::C2Vector pt(evt.x, evt.y);
      if (IsMouseButtonHandled(evt.button) && TestHitRect(pt) && ((evt.button << 8) & m_clickAction)) {
        OnClick(evt.button);
      }

      ASSERT(m_initialized_state != STATE_DELETED);
      ASSERT(m_initialized_state != 0xDDDDDDDD);
      if (!m_stateLocked && m_state != BUTTONSTATE_DISABLED) {
        SetButtonState(BUTTONSTATE_NORMAL, 0);
      }
    }
  }
  return handled;
}

void CSimpleButton::OnDragStart(CMouseEvent &evt) {
  if (m_state != BUTTONSTATE_DISABLED && !m_stateLocked) {
    SetButtonState(BUTTONSTATE_NORMAL, 0);
  }

  CSimpleFrame::OnDragStart(evt);
}

void CSimpleButton::OnLayerCursorEnter() {
  CSimpleFrame::OnLayerCursorEnter();
  UpdateTextState(m_state);

  if (m_trackObserver) {
    s_trackEvent.SetId(m_trackEnterEventId);
    s_trackEvent.SetParam(this);
    m_trackObserver->OnEvent(s_trackEvent);
  }
}

void CSimpleButton::OnLayerCursorExit() {
  CSimpleFrame::OnLayerCursorExit();
  UpdateTextState(m_state);

  if (m_trackObserver) {
    s_trackEvent.SetId(m_trackExitEventId);
    s_trackEvent.SetParam(this);
    m_trackObserver->OnEvent(s_trackEvent);
  }
}

void CSimpleButton::OnClick(MOUSEBUTTON button) {
  if (m_state != BUTTONSTATE_DISABLED) {
    if (m_observer) {
      CSimpleButtonClickEvent evt(m_observerEventId);
      evt.SetParam(this);
      evt.button = button;
      m_observer->OnEvent(evt);
    }

    if (m_onClick) {
      const char *buttonName;

      switch (button) {
        case MOUSE_BUTTON_LEFT:
          buttonName = "LeftButton";
          break;
        case MOUSE_BUTTON_MIDDLE:
          buttonName = "MiddleButton";
          break;
        case MOUSE_BUTTON_RIGHT:
          buttonName = "RightButton";
          break;
        case MOUSE_BUTTON_XBUTTON1:
          buttonName = "Button4";
          break;
        case MOUSE_BUTTON_XBUTTON2:
          buttonName = "Button5";
          break;
        default:
          buttonName = "UNKNOWN";
          break;
      }

      FrameScript_Execute(m_onClick, this, "%s", buttonName);
    }
  }
}

void CSimpleButton::SetClickAction(unsigned int action) {
  m_clickAction = action;
}

void CSimpleButton::RegisterClick(unsigned int eventId, CObserver *observer) {
  m_observer = observer;
  m_observerEventId = eventId;
}

void CSimpleButton::RegisterTrack(unsigned int enterEventId, unsigned int exitEventId, CObserver *observer) {
  m_trackObserver = observer;
  m_trackEnterEventId = enterEventId;
  m_trackExitEventId = exitEventId;
}

void CSimpleButton::SetButtonState(CSimpleButtonState state, int stateLocked) {
  ASSERT(state < NUM_BUTTONSTATES);

  m_stateLocked = stateLocked;
  if (state != m_state) {
    if (m_activeTexture && m_textures[state]) {
      m_activeTexture->Hide();
      m_activeTexture = 0;
    }

    if (m_textures[state]) {
      m_activeTexture = m_textures[state];
      m_activeTexture->Show();
    }

    UpdateTextState(state);
    m_state = state;
  }
}

void CSimpleButton::UpdateTextState(CSimpleButtonState state) {
  CSimpleFontString *text;

  if (m_drawenabled[4] && m_highlightText) {
    text = m_highlightText;
  } else {
    text = m_text;
  }

  if (text) {
    if (state == BUTTONSTATE_PUSHED) {
      text->SetJustificationOffset(m_pressedOffset.x, m_pressedOffset.y);
    } else {
      text->SetJustificationOffset(0.0f, 0.0f);
    }
  }

  if (state == BUTTONSTATE_DISABLED) {
    if (m_disabledText && !m_disabledText->IsVisible()) {
      if (text) {
        text->Hide();
      }
      m_disabledText->Show();
    }
    return;
  }

  if (text && !text->IsVisible()) {
    if (m_disabledText) {
      m_disabledText->Hide();
    }

    if (text == m_text) {
      if (m_highlightText) {
        m_highlightText->Hide();
        text->Show();
        return;
      }
    } else if (m_text) {
      m_text->Hide();
    }

    text->Show();
  }
}

void CSimpleButton::LockHighlight(int lock) {
  if (lock != m_highlightLocked) {
    m_highlightLocked = lock;

    if (lock) {
      EnableDrawLayer(4);
      UpdateTextState(m_state);
      return;
    }

    if (m_top->m_mouseFocus != this) {
      DisableDrawLayer(4);
    }
  }

  UpdateTextState(m_state);
}
