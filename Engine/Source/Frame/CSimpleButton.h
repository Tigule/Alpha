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
  CSimpleButton(CSimpleFrame *parent = 0);
  virtual ~CSimpleButton();
  virtual void LoadXML(const XMLNode *node, CStatus *status);
  virtual void LoadXML_Scripts(const XMLNode *node, CStatus *status);
  void SetText(CSimpleFontString *text);
  void SetDisabledText(CSimpleFontString *text);
  void SetHighlightText(CSimpleFontString *text);

  CSimpleFontString *GetText() {
    return m_text;
  }

  CSimpleFontString *GetDisabledText() {
    return m_disabledText;
  }

  CSimpleFontString *GetHighlightText() {
    return m_highlightText;
  }

  void SetTextString(LPCSTR text);
  void SetDisabledTextString(LPCSTR text);
  void SetHighlightTextString(LPCSTR text);

  LPCSTR GetTextString() {
    LPCSTR text = m_text->GetText();
    return text && *text ? text : 0;
  }

  LPCSTR GetDisabledTextString();
  LPCSTR GetHighlightTextString();
  void SetTextColor(const NTempest::CImVector &color);
  void SetDisabledTextColor(const NTempest::CImVector &color);
  void SetHighlightTextColor(const NTempest::CImVector &color);
  void SetPressedOffset(const NTempest::C2Vector &offset);
  BOOL SetStateTexture(CSimpleButtonState state, LPCSTR texFile);
  void SetStateTexture(CSimpleButtonState state, CSimpleTexture *texture);

  CSimpleTexture *GetStateTexture(CSimpleButtonState state) {
    return m_textures[state];
  }

  virtual void Enable(int enabled);

  BOOL IsEnabled() {
    return m_state != BUTTONSTATE_DISABLED;
  }

  virtual void OnLayerHide();
  virtual BOOL OnLayerMouseDown(CMouseEvent &evt);
  virtual BOOL OnLayerMouseUp(CMouseEvent &evt);
  virtual void OnDragStart(CMouseEvent &evt);
  virtual void OnLayerCursorEnter();
  virtual void OnLayerCursorExit();
  virtual void LockHighlight(int lock);
  virtual void OnClick(MOUSEBUTTON button);
  void SetClickAction(UINT action);

  BOOL IsMouseButtonHandled(MOUSEBUTTON button) {
    return (m_clickAction & (button | (button << 8))) != 0;
  }

  void RegisterClick(UINT eventId, CObserver *observer);
  void RegisterTrack(UINT enterEventId, UINT exitEventId, CObserver *observer);
  virtual void SetButtonState(CSimpleButtonState state, int stateLocked);

  CSimpleButtonState GetButtonState() {
    return m_state;
  }

  void   SetOnClickScript(LPCSTR source);
  void   RunOnClickScript(MOUSEBUTTON button);
  static void RegisterScriptMethods();
  static void UnregisterScriptMethods();

 protected:
  virtual BOOL LookupScriptMethod(lua_State *L, LPCSTR name);
  static TSHashTable<FrameScriptObject_Variable, HASHKEY_STR> s_scriptMethods;
  CObserver         *m_observer;
  UINT               m_observerEventId;
  CObserver         *m_trackObserver;
  UINT               m_trackEnterEventId;
  UINT               m_trackExitEventId;
  CSimpleButtonState m_state;
  int                m_stateLocked;
  UINT               m_clickAction;
  CSimpleFontString *m_disabledText;
  CSimpleFontString *m_text;
  CSimpleFontString *m_highlightText;
  NTempest::C2Vector m_pressedOffset;
  CSimpleTexture    *m_textures[NUM_BUTTONSTATES];
  CSimpleTexture    *m_activeTexture;
  int                m_onClick;
  void UpdateTextState(CSimpleButtonState state);
};

#endif
