#pragma once

#include <DB/WowClientDB.h>

class CinematicCameraRec {
 public:
  CinematicCameraRec();
  ~CinematicCameraRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 7;
  }

  static UINT GetRowSize() {
    return 28;
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

  int    m_ID;
  LPCSTR m_model;
  int    m_soundID;
  float  m_originX;
  float  m_originY;
  float  m_originZ;
  float  m_originFacing;
};

extern WowClientDB<CinematicCameraRec> g_cinematicCameraDB;
