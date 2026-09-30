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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_WMOID) ||
      !SFileReadTyped(f, &m_NameSetID) ||
      !SFileReadTyped(f, &m_WMOGroupID) ||
      !SFileReadTyped(f, &m_DayAmbienceSoundID) ||
      !SFileReadTyped(f, &m_NightAmbienceSoundID) ||
      !SFileReadTyped(f, &m_SoundProviderPref) ||
      !SFileReadTyped(f, &m_SoundProviderPrefUnderwater) ||
      !SFileReadTyped(f, &m_MIDIAmbience) ||
      !SFileReadTyped(f, &m_MIDIAmbienceUnderwater) ||
      !SFileReadTyped(f, &m_ZoneMusic) ||
      !SFileReadTyped(f, &m_IntroSound) ||
      !SFileReadTyped(f, &m_IntroPriority) ||
      !SFileReadTyped(f, &m_Flags) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[0]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[1]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[2]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[3]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[4]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[5]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[6]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[7]) ||
      !SFileReadTyped(f, &m_AreaName_flag)) {
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
