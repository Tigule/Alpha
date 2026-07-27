#include "AreaTableRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *AreaTableRec::GetFilename() {
  return "DBFilesClient\\AreaTable.dbc";
}

AreaTableRec::AreaTableRec() {
}

AreaTableRec::~AreaTableRec() {
}

bool AreaTableRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempAreaName_langIndices[8];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_AreaNumber, sizeof(m_AreaNumber), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ContinentID, sizeof(m_ContinentID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ParentAreaNum, sizeof(m_ParentAreaNum), 0, 0, 0) && result;
  result = SFile::Read(f, &m_AreaBit, sizeof(m_AreaBit), 0, 0, 0) && result;
  result = SFile::Read(f, &m_flags, sizeof(m_flags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundProviderPref, sizeof(m_SoundProviderPref), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SoundProviderPrefUnderwater, sizeof(m_SoundProviderPrefUnderwater), 0, 0, 0) && result;
  result = SFile::Read(f, &m_MIDIAmbience, sizeof(m_MIDIAmbience), 0, 0, 0) && result;
  result = SFile::Read(f, &m_MIDIAmbienceUnderwater, sizeof(m_MIDIAmbienceUnderwater), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ZoneMusic, sizeof(m_ZoneMusic), 0, 0, 0) && result;
  result = SFile::Read(f, &m_IntroSound, sizeof(m_IntroSound), 0, 0, 0) && result;
  result = SFile::Read(f, &m_IntroPriority, sizeof(m_IntroPriority), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[0], sizeof(tempAreaName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[1], sizeof(tempAreaName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[2], sizeof(tempAreaName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[3], sizeof(tempAreaName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[4], sizeof(tempAreaName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[5], sizeof(tempAreaName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[6], sizeof(tempAreaName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempAreaName_langIndices[7], sizeof(tempAreaName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_AreaName_flag, sizeof(m_AreaName_flag), 0, 0, 0) && result;

  if (!result) {
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
