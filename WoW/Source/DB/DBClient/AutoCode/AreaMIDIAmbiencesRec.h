#pragma once

#include <DB/WowClientDB.h>

class AreaMIDIAmbiencesRec {
 public:
  int    m_ID;
  LPCSTR m_DaySequence;
  LPCSTR m_NightSequence;
  LPCSTR m_DLSFile;
  float  m_volume;

  AreaMIDIAmbiencesRec();
  ~AreaMIDIAmbiencesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 5;
  }

  static UINT GetRowSize() {
    return 20;
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
};

extern WowClientDB<AreaMIDIAmbiencesRec> g_areaMIDIAmbiencesDB;
