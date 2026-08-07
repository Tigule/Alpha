#ifndef ENGINE_SOURCE_EVENT_CINPUTOBSERVER_H
#define ENGINE_SOURCE_EVENT_CINPUTOBSERVER_H

#include "CObserver.h"
#include "EvtApi.h"

class CInputObserver : public CObserver {
 public:
  static void RegisterKeyDown(CObserver *pObs);
  static void UnregisterKeyDown(CObserver *pObs);
  static void RegisterKeyDownRepeating(CObserver *pObs);
  static void UnregisterKeyDownRepeating(CObserver *pObs);
  static void RegisterKeyUp(CObserver *pObs);
  static void UnregisterKeyUp(CObserver *pObs);
  static void RegisterChar(CObserver *pObs);
  static void UnregisterChar(CObserver *pObs);
  static void RegisterMouseDown(CObserver *pObs);
  static void UnregisterMouseDown(CObserver *pObs);
  static void RegisterMouseUp(CObserver *pObs);
  static void UnregisterMouseUp(CObserver *pObs);
  static void RegisterMouseMove(CObserver *pObs);
  static void UnregisterMouseMove(CObserver *pObs);
  static void RegisterMouseWheel(CObserver *pObs);
  static void UnregisterMouseWheel(CObserver *pObs);
  static void RegisterMouseMoveRelative(CObserver *pObs);
  static void UnregisterMouseMoveRelative(CObserver *pObs);
  static void RegisterMouseModeChanged(CObserver *pObs);
  static void UnregisterMouseModeChanged(CObserver *pObs);
  static void RegisterIme(CObserver *pObs);
  static void UnregisterIme(CObserver *pObs);
  static void RegisterWindowSize(CObserver *pObs);
  static void UnregisterWindowSize(CObserver *pObs);
  static void RegisterWindowFocus(CObserver *pObs);
  static void UnregisterWindowFocus(CObserver *pObs);
  static void SetMouseMode(MOUSEMODE mode, UINT holdButton);

 private:
  static CInputObserver *GetInputObserver();
  static BOOL            OnChar(const EVENT_DATA_CHAR *pCharEvtData, LPVOID param);
  static BOOL            OnKeyDown(const EVENT_DATA_KEY *pKeyData, LPVOID param);
  static BOOL            OnKeyRepeat(const EVENT_DATA_KEY *pKeyData, LPVOID param);
  static BOOL            OnKeyUp(const EVENT_DATA_KEY *pKeyData, LPVOID param);
  static BOOL            OnMouseDown(const EVENT_DATA_MOUSE *pMouseData, LPVOID param);
  static BOOL            OnMouseUp(const EVENT_DATA_MOUSE *pMouseData, LPVOID param);
  static BOOL            OnMouseMove(const EVENT_DATA_MOUSE *pMouseData, LPVOID param);
  static BOOL            OnMouseWheel(const EVENT_DATA_MOUSE *pMouseData, LPVOID param);
  static BOOL            OnMouseMoveRelative(const EVENT_DATA_MOUSE *pMouseData, LPVOID param);
  static BOOL            OnMouseModeChanged(const EVENT_DATA_MOUSE *pMouseData, LPVOID param);
  static BOOL            OnIme(const EVENT_DATA_IME *pImeData, LPVOID param);
  static BOOL            OnWindowSize(const EVENT_DATA_SIZE *pSizeData, LPVOID param);
  static BOOL            OnWindowFocus(const EVENT_DATA_FOCUS *pFocusData, LPVOID param);
};

#endif
