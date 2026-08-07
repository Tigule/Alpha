#ifndef WOW_SOURCE_UI_ITEMTEXTFRAME_H
#define WOW_SOURCE_UI_ITEMTEXTFRAME_H

#include <stpl.h>

class CGItemText {
 public:
  static void InitializeGame();
  static void ShutdownGame();
  static void EnterWorld();
  static void LeaveWorld();
  static void SetItem(const DWORDLONG &item, int callback);
  static void DisplayText(const DWORDLONG &item, int useSkill);

  static const DWORDLONG &GetItem() {
    return m_itemGUID;
  }
  static UINT GetCurrentPage() {
    return m_currentPage;
  }
  static LPCSTR GetText() {
    return m_text;
  }
  static BOOL HasNextPage() {
    return m_pages[m_currentPage + 1] != 0;
  }
  static void PrevPage();
  static void NextPage();

 private:
  static void                 ItemTextCallback(int id, const DWORDLONG &guid, LPVOID arg, bool granted);
  static DWORDLONG            m_itemGUID;
  static UINT                 m_currentPage;
  static TSGrowableArray<int> m_pages;
  static char                 m_text[0x200];
};

#endif
