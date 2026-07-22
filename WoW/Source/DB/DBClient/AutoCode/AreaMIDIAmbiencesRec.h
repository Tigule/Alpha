#pragma once

#include <DB/WowClientDB.h>

class AreaMIDIAmbiencesRec {
 public:
  AreaMIDIAmbiencesRec();
  ~AreaMIDIAmbiencesRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 5;
  }

  static unsigned int GetRowSize() {
    return 20;
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
  const char *m_DaySequence;
  const char *m_NightSequence;
  const char *m_DLSFile;
  float       m_volume;
};

extern WowClientDB<AreaMIDIAmbiencesRec> g_areaMIDIAmbiencesDB;
