#ifndef ENGINE_SOURCE_EVENT_CINPUTOBSERVER_H
#define ENGINE_SOURCE_EVENT_CINPUTOBSERVER_H

#include "CObserver.h"
#include "EvtApi.h"

class CInputObserver : public CObserver {
 public:
  CInputObserver() {
  }
  CInputObserver(const CInputObserver &);
  virtual ~CInputObserver() {
  }

  CInputObserver &operator=(const CInputObserver &);

  static void __fastcall RegisterKeyDown(CObserver *pObs);
  static void __fastcall UnregisterKeyDown(CObserver *pObs);
  static void __fastcall RegisterKeyDownRepeating(CObserver *pObs);
  static void __fastcall UnregisterKeyDownRepeating(CObserver *pObs);
  static void __fastcall RegisterKeyUp(CObserver *pObs);
  static void __fastcall UnregisterKeyUp(CObserver *pObs);
  static void __fastcall RegisterChar(CObserver *pObs);
  static void __fastcall UnregisterChar(CObserver *pObs);
  static void __fastcall RegisterMouseDown(CObserver *pObs);
  static void __fastcall UnregisterMouseDown(CObserver *pObs);
  static void __fastcall RegisterMouseUp(CObserver *pObs);
  static void __fastcall UnregisterMouseUp(CObserver *pObs);
  static void __fastcall RegisterMouseMove(CObserver *pObs);
  static void __fastcall UnregisterMouseMove(CObserver *pObs);
  static void __fastcall RegisterMouseWheel(CObserver *pObs);
  static void __fastcall UnregisterMouseWheel(CObserver *pObs);
  static void __fastcall RegisterMouseMoveRelative(CObserver *pObs);
  static void __fastcall UnregisterMouseMoveRelative(CObserver *pObs);
  static void __fastcall RegisterMouseModeChanged(CObserver *pObs);
  static void __fastcall UnregisterMouseModeChanged(CObserver *pObs);
  static void __fastcall RegisterIme(CObserver *pObs);
  static void __fastcall UnregisterIme(CObserver *pObs);
  static void __fastcall RegisterWindowSize(CObserver *pObs);
  static void __fastcall UnregisterWindowSize(CObserver *pObs);
  static void __fastcall RegisterWindowFocus(CObserver *pObs);
  static void __fastcall UnregisterWindowFocus(CObserver *pObs);
  static void __fastcall SetMouseMode(MOUSEMODE mode, unsigned int holdButton);

 private:
  static CInputObserver *__fastcall GetInputObserver();
  static int __fastcall OnChar(const EVENT_DATA_CHAR *pCharEvtData, void *param);
  static int __fastcall OnKeyDown(const EVENT_DATA_KEY *pKeyData, void *param);
  static int __fastcall OnKeyRepeat(const EVENT_DATA_KEY *pKeyData, void *param);
  static int __fastcall OnKeyUp(const EVENT_DATA_KEY *pKeyData, void *param);
  static int __fastcall OnMouseDown(const EVENT_DATA_MOUSE *pMouseData, void *param);
  static int __fastcall OnMouseUp(const EVENT_DATA_MOUSE *pMouseData, void *param);
  static int __fastcall OnMouseMove(const EVENT_DATA_MOUSE *pMouseData, void *param);
  static int __fastcall OnMouseWheel(const EVENT_DATA_MOUSE *pMouseData, void *param);
  static int __fastcall OnMouseMoveRelative(const EVENT_DATA_MOUSE *pMouseData, void *param);
  static int __fastcall OnMouseModeChanged(const EVENT_DATA_MOUSE *pMouseData, void *param);
  static int __fastcall OnIme(const EVENT_DATA_IME *pImeData, void *param);
  static int __fastcall OnWindowSize(const EVENT_DATA_SIZE *pSizeData, void *param);
  static int __fastcall OnWindowFocus(const EVENT_DATA_FOCUS *pFocusData, void *param);
};

#endif
