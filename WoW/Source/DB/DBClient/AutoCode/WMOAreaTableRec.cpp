#include "WMOAreaTableRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR WMOAreaTableRec::GetFilename() {
  return "DBFilesClient\\WMOAreaTable.dbc";
}

WMOAreaTableRec::WMOAreaTableRec() {
}

WMOAreaTableRec::~WMOAreaTableRec() {
}

bool WMOAreaTableRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempAreaName_langIndices[NUM_LOCALES];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_WMOID) == 0);
  error |= (SFileReadTyped(f, &m_NameSetID) == 0);
  error |= (SFileReadTyped(f, &m_WMOGroupID) == 0);
  error |= (SFileReadTyped(f, &m_DayAmbienceSoundID) == 0);
  error |= (SFileReadTyped(f, &m_NightAmbienceSoundID) == 0);
  error |= (SFileReadTyped(f, &m_SoundProviderPref) == 0);
  error |= (SFileReadTyped(f, &m_SoundProviderPrefUnderwater) == 0);
  error |= (SFileReadTyped(f, &m_MIDIAmbience) == 0);
  error |= (SFileReadTyped(f, &m_MIDIAmbienceUnderwater) == 0);
  error |= (SFileReadTyped(f, &m_ZoneMusic) == 0);
  error |= (SFileReadTyped(f, &m_IntroSound) == 0);
  error |= (SFileReadTyped(f, &m_IntroPriority) == 0);
  error |= (SFileReadTyped(f, &m_Flags) == 0);
  error |= (SFileReadTyped(f, &tempAreaName_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempAreaName_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempAreaName_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempAreaName_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempAreaName_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempAreaName_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempAreaName_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempAreaName_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_AreaName_flag) == 0);

  if (error) {
    ConsoleWrite("Error reading WMOAreaTableRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_AreaName_lang[0] = &stringBuffer[tempAreaName_langIndices[0]];
    m_AreaName_lang[1] = &stringBuffer[tempAreaName_langIndices[1]];
    m_AreaName_lang[2] = &stringBuffer[tempAreaName_langIndices[2]];
    m_AreaName_lang[3] = &stringBuffer[tempAreaName_langIndices[3]];
    m_AreaName_lang[4] = &stringBuffer[tempAreaName_langIndices[4]];
    m_AreaName_lang[5] = &stringBuffer[tempAreaName_langIndices[5]];
    m_AreaName_lang[6] = &stringBuffer[tempAreaName_langIndices[6]];
    m_AreaName_lang[7] = &stringBuffer[tempAreaName_langIndices[7]];
  } else {
    m_AreaName_lang[0] = "";
    m_AreaName_lang[1] = "";
    m_AreaName_lang[2] = "";
    m_AreaName_lang[3] = "";
    m_AreaName_lang[4] = "";
    m_AreaName_lang[5] = "";
    m_AreaName_lang[6] = "";
    m_AreaName_lang[7] = "";
  }

  return true;
}
