#include "AreaTableRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR AreaTableRec::GetFilename() {
  return "DBFilesClient\\AreaTable.dbc";
}

AreaTableRec::AreaTableRec() {
}

AreaTableRec::~AreaTableRec() {
}

bool AreaTableRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempAreaName_langIndices[8];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_AreaNumber) ||
      !SFileReadTyped(f, &m_ContinentID) ||
      !SFileReadTyped(f, &m_ParentAreaNum) ||
      !SFileReadTyped(f, &m_AreaBit) ||
      !SFileReadTyped(f, &m_flags) ||
      !SFileReadTyped(f, &m_SoundProviderPref) ||
      !SFileReadTyped(f, &m_SoundProviderPrefUnderwater) ||
      !SFileReadTyped(f, &m_MIDIAmbience) ||
      !SFileReadTyped(f, &m_MIDIAmbienceUnderwater) ||
      !SFileReadTyped(f, &m_ZoneMusic) ||
      !SFileReadTyped(f, &m_IntroSound) ||
      !SFileReadTyped(f, &m_IntroPriority) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[0]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[1]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[2]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[3]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[4]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[5]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[6]) ||
      !SFileReadTyped(f, &tempAreaName_langIndices[7]) ||
      !SFileReadTyped(f, &m_AreaName_flag)) {
    ConsoleWrite("Error reading AreaTableRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_AreaName_lang[0] = stringBuffer + tempAreaName_langIndices[0];
    m_AreaName_lang[1] = stringBuffer + tempAreaName_langIndices[1];
    m_AreaName_lang[2] = stringBuffer + tempAreaName_langIndices[2];
    m_AreaName_lang[3] = stringBuffer + tempAreaName_langIndices[3];
    m_AreaName_lang[4] = stringBuffer + tempAreaName_langIndices[4];
    m_AreaName_lang[5] = stringBuffer + tempAreaName_langIndices[5];
    m_AreaName_lang[6] = stringBuffer + tempAreaName_langIndices[6];
    m_AreaName_lang[7] = stringBuffer + tempAreaName_langIndices[7];
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
