#pragma once

#include <DB/WowClientDB.h>

class CharacterCreateCamerasRec {
 public:
  CharacterCreateCamerasRec();
  ~CharacterCreateCamerasRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 6;
  }

  static unsigned int GetRowSize() {
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

  bool Read(SFile *f, const char *stringBuffer);

  int   m_Race;
  int   m_Sex;
  int   m_Camera;
  float m_Height;
  float m_Radius;
  float m_Target;
  int   m_generatedID;
};

extern WowClientDB<CharacterCreateCamerasRec> g_characterCreateCamerasDB;
