#ifndef ENGINE_SOURCE_EVENT_CMOUSEEVENT_H
#define ENGINE_SOURCE_EVENT_CMOUSEEVENT_H

#include "Base/Coordinate.h"
#include "Base/RefCount.h"
#include "Event/EvtApi.h"

class CEvent : public TRefCnt {
 private:
  UINT   id;
  LPVOID param;

 public:
  CEvent(UINT id = static_cast<UINT>(-1), LPVOID param = 0) : id(id), param(param) {
  }

  virtual ~CEvent() {
  }

  UINT Id() const {
    return id;
  }

  void SetId(UINT value) {
    id = value;
  }

  LPVOID GetParam() const {
    return param;
  }

  void SetParam(LPVOID value) {
    param = value;
  }
};

class CCharEvent : public CEvent, public EVENT_DATA_CHAR {
 public:
  CCharEvent() {
  }

  CCharEvent(const EVENT_DATA_CHAR &data) {
    static_cast<EVENT_DATA_CHAR &>(*this) = data;
  }

  CCharEvent &operator=(const EVENT_DATA_CHAR &data) {
    static_cast<EVENT_DATA_CHAR &>(*this) = data;
    return *this;
  }

  virtual ~CCharEvent() {
  }

  static BOOL IsShiftDown();
  static BOOL IsControlDown();
  static BOOL IsAltDown();
};

class CImeEvent : public CEvent, public EVENT_DATA_IME {
 public:
  CImeEvent() {
  }

  CImeEvent(const EVENT_DATA_IME &data) {
    static_cast<EVENT_DATA_IME &>(*this) = data;
  }

  CImeEvent &operator=(const EVENT_DATA_IME &data) {
    static_cast<EVENT_DATA_IME &>(*this) = data;
    return *this;
  }

  virtual ~CImeEvent() {
  }
};

class CFocusEvent : public CEvent, public EVENT_DATA_FOCUS {
 public:
  CFocusEvent() {
  }

  CFocusEvent(const EVENT_DATA_FOCUS &data) {
    static_cast<EVENT_DATA_FOCUS &>(*this) = data;
  }

  CFocusEvent &operator=(const EVENT_DATA_FOCUS &data) {
    static_cast<EVENT_DATA_FOCUS &>(*this) = data;
    return *this;
  }

  virtual ~CFocusEvent() {
  }
};

class CKeyEvent : public CEvent, public EVENT_DATA_KEY {
 public:
  CKeyEvent() {
  }

  CKeyEvent(const EVENT_DATA_KEY &data) {
    static_cast<EVENT_DATA_KEY &>(*this) = data;
  }

  CKeyEvent &operator=(const EVENT_DATA_KEY &data) {
    static_cast<EVENT_DATA_KEY &>(*this) = data;
    return *this;
  }

  virtual ~CKeyEvent() {
  }

  static BOOL IsShiftDown();
  static BOOL IsControlDown();
  static BOOL IsAltDown();
};

class CMouseEvent : public CEvent, public EVENT_DATA_MOUSE {
 public:
  CMouseEvent() {
  }

  CMouseEvent(const EVENT_DATA_MOUSE &data) {
    mode = data.mode;
    button = data.button;
    buttonState = data.buttonState;
    metaKeyState = data.metaKeyState;
    flags = data.flags;
    time = data.time;
    wheelDistance = data.wheelDistance;
    NDCToDDC(data.x, data.y, &x, &y);
  }

  CMouseEvent &operator=(const EVENT_DATA_MOUSE &rhs);

  virtual ~CMouseEvent() {
  }
};

class CSizeEvent : public CEvent, public EVENT_DATA_SIZE {
 public:
  CSizeEvent() {
  }

  CSizeEvent(const EVENT_DATA_SIZE &data) {
    static_cast<EVENT_DATA_SIZE &>(*this) = data;
  }

  CSizeEvent &operator=(const EVENT_DATA_SIZE &data) {
    static_cast<EVENT_DATA_SIZE &>(*this) = data;
    return *this;
  }

  virtual ~CSizeEvent() {
  }
};

#endif
