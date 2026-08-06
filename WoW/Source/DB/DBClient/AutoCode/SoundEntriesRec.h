#pragma once

#include <DB/WowClientDB.h>

class SoundEntriesRec {
 public:
  SoundEntriesRec();
  ~SoundEntriesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 34;
  }

  static UINT GetRowSize() {
    return 136;
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
  int    m_soundType;
  LPCSTR m_name;
  LPCSTR m_File[10];
  int    m_Freq[10];
  LPCSTR m_DirectoryBase;
  float  m_volumeFloat;
  float  m_pitch;
  float  m_pitchVariation;
  int    m_priority;
  int    m_channel;
  int    m_flags;
  float  m_minDistance;
  float  m_maxDistance;
  float  m_distanceCutoff;
  int    m_EAXDef;
};

extern WowClientDB<SoundEntriesRec> g_soundEntriesDB;
