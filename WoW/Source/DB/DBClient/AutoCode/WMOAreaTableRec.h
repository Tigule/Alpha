#pragma once

#include <DB/WowClientDB.h>

class WMOAreaTableRec {
 public:
  int    m_ID;
  int    m_WMOID;
  int    m_NameSetID;
  int    m_WMOGroupID;
  int    m_DayAmbienceSoundID;
  int    m_NightAmbienceSoundID;
  int    m_SoundProviderPref;
  int    m_SoundProviderPrefUnderwater;
  int    m_MIDIAmbience;
  int    m_MIDIAmbienceUnderwater;
  int    m_ZoneMusic;
  int    m_IntroSound;
  int    m_IntroPriority;
  int    m_Flags;
  LPCSTR m_AreaName_lang[NUM_LOCALES];
  int    m_AreaName_flag;

  WMOAreaTableRec();
  ~WMOAreaTableRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 23;
  }

  static UINT GetRowSize() {
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

  bool Read(SFile *f, LPCSTR stringBuffer);
};

extern WowClientDB<WMOAreaTableRec> g_wMOAreaTableDB;
