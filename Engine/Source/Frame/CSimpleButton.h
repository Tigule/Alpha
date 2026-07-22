#ifndef ENGINE_SOURCE_FRAME_CSIMPLEBUTTON_H
#define ENGINE_SOURCE_FRAME_CSIMPLEBUTTON_H

#include "Frame/CSimpleFrame.h"
#include "Tempest/c2vector.h"

class CObserver;
class CSimpleFontString;
class CSimpleTexture;

enum CSimpleButtonState {
  BUTTONSTATE_DISABLED = 0,
  BUTTONSTATE_NORMAL = 1,
  BUTTONSTATE_PUSHED = 2,
  NUM_BUTTONSTATES = 3
};

class CSimpleButton : public CSimpleFrame {
 public:
  CSimpleButton(CSimpleFrame *parent);
  virtual ~CSimpleButton();

  static void __fastcall RegisterScriptMethods();
  static void __fastcall UnregisterScriptMethods();

  virtual void LoadXML(const XMLNode *node, CStatus *status);
  virtual void LoadXML_Scripts(const XMLNode *node, CStatus *status);
  virtual void Enable(int enabled);
  virtual void OnLayerHide();
  virtual int  OnLayerMouseDown(CMouseEvent &evt);
  virtual int  OnLayerMouseUp(CMouseEvent &evt);
  virtual void OnDragStart(CMouseEvent &evt);
  virtual void OnLayerCursorEnter();
  virtual void OnLayerCursorExit();
  virtual void LockHighlight(int lock);
  virtual void OnClick(MOUSEBUTTON button);
  virtual void SetButtonState(CSimpleButtonState state, int stateLocked);

  void SetText(CSimpleFontString *text);
  void SetDisabledText(CSimpleFontString *text);
  void SetHighlightText(CSimpleFontString *text);
  void SetTextString(const char *text);
  void SetDisabledTextString(const char *text);
  void SetHighlightTextString(const char *text);
  void SetTextColor(const NTempest::CImVector &color);
  void SetDisabledTextColor(const NTempest::CImVector &color);
  void SetHighlightTextColor(const NTempest::CImVector &color);
  void SetPressedOffset(const NTempest::C2Vector &offset);
  void SetStateTexture(CSimpleButtonState state, CSimpleTexture *texture);
  int  SetStateTexture(CSimpleButtonState state, const char *texFile);
  void SetClickAction(unsigned int action);
  int  IsMouseButtonHandled(MOUSEBUTTON button) {
    return (m_clickAction & (button | (button << 8))) != 0;
  }
  void RegisterClick(unsigned int eventId, CObserver *observer);
  void RegisterTrack(unsigned int enterEventId, unsigned int exitEventId, CObserver *observer);

  CSimpleFontString *GetText() {
    return m_text;
  }

  CSimpleFontString *GetDisabledText() {
    return m_disabledText;
  }

  CSimpleFontString *GetHighlightText() {
    return m_highlightText;
  }

  const char *GetTextString() {
    const char *text = m_text->GetText();
    return text && *text ? text : 0;
  }

  CSimpleTexture *GetStateTexture(CSimpleButtonState state) {
    return m_textures[state];
  }

  int IsEnabled() {
    return m_state != BUTTONSTATE_DISABLED;
  }

  CSimpleButtonState GetButtonState() {
    return m_state;
  }

 protected:
  virtual int LookupScriptMethod(lua_State *L, const char *name);

  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;

  void UpdateTextState(CSimpleButtonState state);

  CObserver         *m_observer;
  unsigned int       m_observerEventId;
  CObserver         *m_trackObserver;
  unsigned int       m_trackEnterEventId;
  unsigned int       m_trackExitEventId;
  CSimpleButtonState m_state;
  int                m_stateLocked;
  unsigned int       m_clickAction;
  CSimpleFontString *m_disabledText;
  CSimpleFontString *m_text;
  CSimpleFontString *m_highlightText;
  NTempest::C2Vector m_pressedOffset;
  CSimpleTexture    *m_textures[NUM_BUTTONSTATES];
  CSimpleTexture    *m_activeTexture;
  int                m_onClick;
};

#endif
