#ifndef WOW_SOURCE_UI_TABARDCREATIONFRAME_H
#define WOW_SOURCE_UI_TABARDCREATIONFRAME_H

class CGTabardCreationFrame {
 public:
  static void EnterWorld();
  static void LeaveWorld();
  static void Open(const unsigned __int64 &vendor);
  static void Close();
  static void ClearVendor() {
    m_vendor = 0;
  }
  static unsigned __int64 GetVendor() {
    return m_vendor;
  }

 private:
  static unsigned __int64 m_vendor;
};

#endif
