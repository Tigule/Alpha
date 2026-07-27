#pragma once

#include <DB/WowClientDB.h>

class CinematicCameraRec {
 public:
  CinematicCameraRec();
  ~CinematicCameraRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 7;
  }

  static unsigned int GetRowSize() {
    return 28;
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

  int         m_ID;
  const char *m_model;
  int         m_soundID;
  float       m_originX;
  float       m_originY;
  float       m_originZ;
  float       m_originFacing;
};

extern WowClientDB<CinematicCameraRec> g_cinematicCameraDB;
