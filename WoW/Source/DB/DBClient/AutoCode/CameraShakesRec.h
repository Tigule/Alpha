#pragma once

#include <DB/WowClientDB.h>

class CameraShakesRec {
 public:
  CameraShakesRec();
  ~CameraShakesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 8;
  }

  static UINT GetRowSize() {
    return 32;
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

  int   m_ID;
  int   m_shakeType;
  int   m_direction;
  float m_amplitude;
  float m_frequency;
  float m_duration;
  float m_phase;
  float m_coefficient;
};

extern WowClientDB<CameraShakesRec> g_cameraShakesDB;
