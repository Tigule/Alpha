#ifndef WOW_SOURCE_UIUTIL_CURSOR_H
#define WOW_SOURCE_UIUTIL_CURSOR_H

#include <Base/Handle.h>

enum CURSORITEMTYPE {
  CURSOR_EMPTY = 0,
  CURSOR_ITEM = 1,
  CURSOR_MONEY = 2,
  CURSOR_SPELL = 3,
  NUM_CURSOR_ITEM_TYPES = 4
};

enum CURSORANIMATIONS {
  POINT_CURSOR = 0,
  CAST_CURSOR = 1,
  BUY_CURSOR = 2,
  ATTACK_CURSOR = 3,
  INTERACT_CURSOR = 4,
  SPEAK_CURSOR = 5,
  RANGED_CURSOR = 6,
  PICKUP_CURSOR = 7,
  TAXI_CURSOR = 8,
  ERROR_CURSORS = 9,
  POINT_ERROR_CURSOR = 9,
  CAST_ERROR_CURSOR = 10,
  BUY_ERROR_CURSOR = 11,
  ATTACK_ERROR_CURSOR = 12,
  INTERACT_ERROR_CURSOR = 13,
  SPEAK_ERROR_CURSOR = 14,
  RANGED_ERROR_CURSOR = 15,
  PICKUP_ERROR_CURSOR = 16,
  TAXI_ERROR_CURSOR = 17,
  NUM_CURSOR_ANIMS = 18,
  NO_CURSOR = 19
};

class CGCursor {
 public:
  CGCursor() : m_model(0), m_heldItem(CURSOR_EMPTY), m_cursorMode(POINT_CURSOR), m_mouseOver(NO_CURSOR) {
  }

  CGCursor(const CGCursor &);
  ~CGCursor();

  void SetArt(LPCSTR art);
  void Drop();
  void Grab(HMODEL model);

  HMODEL GetModel() {
    return m_model;
  }

  CURSORITEMTYPE GetItemType() {
    return m_heldItem;
  }

  void SetItemType(CURSORITEMTYPE type);
  void SetCursorAnim(CURSORANIMATIONS sequence);
  void SetCursorMode(CURSORANIMATIONS sequence);
  void ResetCursor();

 private:
  CGCursor &operator=(const CGCursor &);

  HMODEL           m_model;
  CURSORITEMTYPE   m_heldItem;
  CURSORANIMATIONS m_cursorMode;
  CURSORANIMATIONS m_mouseOver;
};

extern CGCursor *g_cursor;

void CursorInitialize();
void CursorDestroy();
BOOL CursorGrabSpell(HMODEL model);
BOOL CursorGrabSpell(LPCSTR filename);
void CursorDropMoney();
void CursorDropSpell();
void CursorModelSetSequence(CURSORANIMATIONS sequence);

#endif
