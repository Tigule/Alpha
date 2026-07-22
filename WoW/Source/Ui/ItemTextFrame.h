#ifndef WOW_SOURCE_UI_ITEMTEXTFRAME_H
#define WOW_SOURCE_UI_ITEMTEXTFRAME_H

#include <stpl.h>

class CGItemText {
 public:
  static void __fastcall InitializeGame();
  static void __fastcall ShutdownGame();
  static void __fastcall EnterWorld();
  static void __fastcall LeaveWorld();
  static void __fastcall SetItem(const unsigned __int64 &item, int callback);

  static const unsigned __int64 &GetItem() {
    return m_itemGUID;
  }
  static unsigned int GetCurrentPage() {
    return m_currentPage;
  }
  static const char *GetText() {
    return m_text;
  }
  static int HasNextPage() {
    return m_pages[m_currentPage + 1] != 0;
  }
  static void __fastcall PrevPage();
  static void __fastcall NextPage();

 private:
  static void __fastcall      ItemTextCallback(int id, const unsigned __int64 &guid, void *arg, bool granted);
  static void __fastcall      DisplayText(const unsigned __int64 &item, int useSkill);
  static unsigned __int64     m_itemGUID;
  static unsigned int         m_currentPage;
  static TSGrowableArray<int> m_pages;
  static char                 m_text[0x200];
};

#endif
