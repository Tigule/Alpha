#pragma once

#include <DB/WowClientDB.h>

class AreaTableRec {
 public:
  AreaTableRec();
  ~AreaTableRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 22;
  }

  static unsigned int GetRowSize() {
    return 88;
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
  int         m_AreaNumber;
  int         m_ContinentID;
  int         m_ParentAreaNum;
  int         m_AreaBit;
  int         m_flags;
  int         m_SoundProviderPref;
  int         m_SoundProviderPrefUnderwater;
  int         m_MIDIAmbience;
  int         m_MIDIAmbienceUnderwater;
  int         m_ZoneMusic;
  int         m_IntroSound;
  int         m_IntroPriority;
  const char *m_AreaName_lang[NUM_LOCALES];
  int         m_AreaName_flag;
};

extern WowClientDB<AreaTableRec> g_areaTableDB;
