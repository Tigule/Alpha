#pragma once

#include <DB/WowClientDB.h>

class CharacterCreateCamerasRec {
 public:
  int   m_Race;
  int   m_Sex;
  int   m_Camera;
  float m_Height;
  float m_Radius;
  float m_Target;
  int   m_generatedID;

  CharacterCreateCamerasRec();
  ~CharacterCreateCamerasRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 6;
  }

  static UINT GetRowSize() {
    return 24;
  }

  int GetID() const {
    return m_generatedID;
  }

  bool NeedIDAssigned() {
    return true;
  }

  void SetID(int id) {
    m_generatedID = id;
  }

  bool Read(SFile *f, LPCSTR stringBuffer);
};

extern WowClientDB<CharacterCreateCamerasRec> g_characterCreateCamerasDB;
