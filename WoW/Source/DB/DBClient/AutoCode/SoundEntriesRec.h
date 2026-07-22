#pragma once

#include <DB/WowClientDB.h>

class SoundEntriesRec {
 public:
  SoundEntriesRec();
  ~SoundEntriesRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 34;
  }

  static unsigned int GetRowSize() {
    return 136;
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
  int         m_soundType;
  const char *m_name;
  const char *m_File[10];
  int         m_Freq[10];
  const char *m_DirectoryBase;
  float       m_volumeFloat;
  float       m_pitch;
  float       m_pitchVariation;
  int         m_priority;
  int         m_channel;
  int         m_flags;
  float       m_minDistance;
  float       m_maxDistance;
  float       m_distanceCutoff;
  int         m_EAXDef;
};

extern WowClientDB<SoundEntriesRec> g_soundEntriesDB;
