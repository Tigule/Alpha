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
  TSLink<CSimpleHyperlinkButton> m_link;
};

class CSimpleHyperlinkedFrame : public CSimpleFrame {
 public:
  CSimpleHyperlinkedFrame(CSimpleFrame *parent = 0);
  virtual ~CSimpleHyperlinkedFrame();

  virtual void LoadXML_Scripts(const XMLNode *node, CStatus *status);
  virtual void OnHyperlinkEnter(const char *link);
  virtual void OnHyperlinkLeave(const char *link);
  virtual void OnHyperlinkClick(const char *link, MOUSEBUTTON button);

  void SetOnHyperlinkEnterScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnHyperlinkEnter", GetName());
    SetEventScript(m_onHyperlinkEnter, source, description);
  }

  void SetOnHyperlinkLeaveScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnHyperlinkLeave", GetName());
    SetEventScript(m_onHyperlinkLeave, source, description);
  }

  void SetOnHyperlinkClickScript(const char *source) {
    char description[1024];
    SStrPrintf(description, sizeof(description), "%s:OnHyperlinkClick", GetName());
    SetEventScript(m_onHyperlinkClick, source, description);
  }

  void RunOnHyperlinkEnterScript(const char *link) {
    if (m_onHyperlinkEnter) {
      FrameScript_Execute(m_onHyperlinkEnter, this, "%s", link);
    }
  }

  void RunOnHyperlinkLeaveScript(const char *link) {
    if (m_onHyperlinkLeave) {
      FrameScript_Execute(m_onHyperlinkLeave, this, "%s", link);
    }
  }

  void RunOnHyperlinkClickScript(const char *link, MOUSEBUTTON button) {
    if (m_onHyperlinkClick) {
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

      FrameScript_Execute(m_onHyperlinkClick, this, "%s%s", link, buttonName);
    }
  }

 protected:
  CSimpleHyperlinkButton *CreateHyperlinkButton();
  void                    ReleaseHyperlinkButton(CSimpleHyperlinkButton *button);

  TSExplicitList<CSimpleHyperlinkButton, 760> m_hyperlinkButtons;
  int                                         m_onHyperlinkEnter;
  int                                         m_onHyperlinkLeave;
  int                                         m_onHyperlinkClick;
};

#endif
