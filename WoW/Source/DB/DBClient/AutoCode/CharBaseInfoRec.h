#pragma once

#include <DB/WowClientDB.h>

class CharBaseInfoRec {
 public:
  CharBaseInfoRec();
  ~CharBaseInfoRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 3;
  }

  static unsigned int GetRowSize() {
    return 6;
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

  unsigned char m_raceID;
  unsigned char m_classID;
  int           m_proficiency;
  int           m_generatedID;
};

extern WowClientDB<CharBaseInfoRec> g_charBaseInfoDB;
