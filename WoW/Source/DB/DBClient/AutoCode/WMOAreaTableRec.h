#pragma once

#include <DB/WowClientDB.h>

class WMOAreaTableRec {
 public:
  WMOAreaTableRec();
  ~WMOAreaTableRec();

  static const char *GetFilename();

  static unsigned int GetNumColumns() {
    return 23;
  }

  static unsigned int GetRowSize() {
    return 92;
  }

  int GetID() const {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  int         m_WMOID;
  int         m_NameSetID;
  int         m_WMOGroupID;
  int         m_DayAmbienceSoundID;
  int         m_NightAmbienceSoundID;
  int         m_SoundProviderPref;
  int         m_SoundProviderPrefUnderwater;
  int         m_MIDIAmbience;
  int         m_MIDIAmbienceUnderwater;
  int         m_ZoneMusic;
  int         m_IntroSound;
  int         m_IntroPriority;
  int         m_Flags;
  const char *m_AreaName_lang[NUM_LOCALES];
  int         m_AreaName_flag;
};

extern WowClientDB<WMOAreaTableRec> g_wMOAreaTableDB;
