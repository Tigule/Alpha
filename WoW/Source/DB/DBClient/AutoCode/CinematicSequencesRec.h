#pragma once

#include <DB/WowClientDB.h>

class CinematicSequencesRec {
 public:
  CinematicSequencesRec();
  ~CinematicSequencesRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 10;
  }

  static unsigned int GetRowSize() {
    return 40;
  }

  int GetID() {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int m_ID;
  int m_soundID;
  int m_camera[8];
};

extern WowClientDB<CinematicSequencesRec> g_cinematicSequencesDB;
