#ifndef ENGINE_SOURCE_FRAME_CSIMPLEHYPERLINKEDFRAME_H
#define ENGINE_SOURCE_FRAME_CSIMPLEHYPERLINKEDFRAME_H

#include "Frame/CSimpleButton.h"

#include <stpl.h>

class CSimpleFontString;
class CSimpleHyperlinkedFrame;
struct GXUFONTHYPERLINKINFO;

class CSimpleHyperlinkButton : public CSimpleButton {
 public:
  CSimpleHyperlinkButton(CSimpleHyperlinkedFrame *parent);
  virtual ~CSimpleHyperlinkButton();

  void SetHyperlink(CSimpleFontString *string, const GXUFONTHYPERLINKINFO *hyperlink);

  virtual void OnLayerCursorEnter();
  virtual void OnLayerCursorExit();
  virtual void OnClick(MOUSEBUTTON button);

 protected:
  char *m_hyperlink;

 public:
  LINKDECLEX(CSimpleHyperlinkButton, m_link);
};

class CSimpleHyperlinkedFrame : public CSimpleFrame {
 public:
  CSimpleHyperlinkedFrame(CSimpleFrame *parent = 0);
  virtual ~CSimpleHyperlinkedFrame();

  virtual void LoadXML_Scripts(const XMLNode *node, CStatus *status);
  virtual void OnHyperlinkEnter(LPCSTR link);
  virtual void OnHyperlinkLeave(LPCSTR link);
  virtual void OnHyperlinkClick(LPCSTR link, MOUSEBUTTON button);

  void SetOnHyperlinkEnterScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnHyperlinkEnter", GetName());
    SetEventScript(m_onHyperlinkEnter, source, description);
  }

  void SetOnHyperlinkLeaveScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnHyperlinkLeave", GetName());
    SetEventScript(m_onHyperlinkLeave, source, description);
  }

  void SetOnHyperlinkClickScript(LPCSTR source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnHyperlinkClick", GetName());
    SetEventScript(m_onHyperlinkClick, source, description);
  }

  void RunOnHyperlinkEnterScript(LPCSTR link) {
    if (m_onHyperlinkEnter) {
      FrameScript_Execute(m_onHyperlinkEnter, this, "%s", link);
    }
  }

  void RunOnHyperlinkLeaveScript(LPCSTR link) {
    if (m_onHyperlinkLeave) {
      FrameScript_Execute(m_onHyperlinkLeave, this, "%s", link);
    }
  }

  void RunOnHyperlinkClickScript(LPCSTR link, MOUSEBUTTON button) {
    if (m_onHyperlinkClick) {
      LPCSTR buttonName;

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

      FrameScript_Execute(m_onHyperlinkClick, this, "%s%s", link, buttonName);
    }
  }

 protected:
  CSimpleHyperlinkButton *CreateHyperlinkButton();
  void                    ReleaseHyperlinkButton(CSimpleHyperlinkButton *button);

  LISTDECLEX(CSimpleHyperlinkButton, m_link, m_hyperlinkButtons);
  int m_onHyperlinkEnter;
  int m_onHyperlinkLeave;
  int m_onHyperlinkClick;
};

#endif
