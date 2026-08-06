#pragma once

#include <DB/WowClientDB.h>

class CinematicSequencesRec {
 public:
  CinematicSequencesRec();
  ~CinematicSequencesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 10;
  }

  static UINT GetRowSize() {
    return 40;
  }

  int GetID() const {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, LPCSTR stringBuffer);

  int m_ID;
  int m_soundID;
  int m_camera[8];
};

extern WowClientDB<CinematicSequencesRec> g_cinematicSequencesDB;
