#ifndef WOW_SOURCE_UI_TABARDCREATIONFRAME_H
#define WOW_SOURCE_UI_TABARDCREATIONFRAME_H

class CGTabardCreationFrame {
 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void Open(const DWORDLONG &vendor);
  static void Close();
  static void ClearVendor() {
    m_vendor = 0;
  }
  static DWORDLONG GetVendor() {
    return m_vendor;
  }

 private:
  static DWORDLONG m_vendor;
};

#endif
