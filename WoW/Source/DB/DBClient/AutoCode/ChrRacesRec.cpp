#include "ChrRacesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR ChrRacesRec::GetFilename() {
  return "DBFilesClient\\ChrRaces.dbc";
}

ChrRacesRec::ChrRacesRec() {
}

ChrRacesRec::~ChrRacesRec() {
}

bool ChrRacesRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempname_langIndices[NUM_LOCALES];
  UINT tempclientFileStringIndices[1];
  UINT tempClientPrefixIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_flags, sizeof(m_flags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_factionID, sizeof(m_factionID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_MaleDisplayId, sizeof(m_MaleDisplayId), 0, 0, 0) && result;
  result = SFile::Read(f, &m_FemaleDisplayId, sizeof(m_FemaleDisplayId), 0, 0, 0) && result;
  result = SFile::Read(f, &tempClientPrefixIndices[0], sizeof(tempClientPrefixIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_MountScale, sizeof(m_MountScale), 0, 0, 0) && result;
  result = SFile::Read(f, &m_BaseLanguage, sizeof(m_BaseLanguage), 0, 0, 0) && result;
  result = SFile::Read(f, &m_creatureType, sizeof(m_creatureType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_LoginEffectSpellID, sizeof(m_LoginEffectSpellID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_CombatStunSpellID, sizeof(m_CombatStunSpellID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ResSicknessSpellID, sizeof(m_ResSicknessSpellID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SplashSoundID, sizeof(m_SplashSoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_startingTaxiNodes, sizeof(m_startingTaxiNodes), 0, 0, 0) && result;
  result = SFile::Read(f, &tempclientFileStringIndices[0], sizeof(tempclientFileStringIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_cinematicSequenceID, sizeof(m_cinematicSequenceID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[0], sizeof(tempname_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[1], sizeof(tempname_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[2], sizeof(tempname_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[3], sizeof(tempname_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[4], sizeof(tempname_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[5], sizeof(tempname_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[6], sizeof(tempname_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[7], sizeof(tempname_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_name_flag, sizeof(m_name_flag), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading ChrRacesRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_ClientPrefix = stringBuffer + tempClientPrefixIndices[0];
    m_clientFileString = stringBuffer + tempclientFileStringIndices[0];
    m_name_lang[0] = stringBuffer + tempname_langIndices[0];
    m_name_lang[1] = stringBuffer + tempname_langIndices[1];
    m_name_lang[2] = stringBuffer + tempname_langIndices[2];
    m_name_lang[3] = stringBuffer + tempname_langIndices[3];
    m_name_lang[4] = stringBuffer + tempname_langIndices[4];
    m_name_lang[5] = stringBuffer + tempname_langIndices[5];
    m_name_lang[6] = stringBuffer + tempname_langIndices[6];
    m_name_lang[7] = stringBuffer + tempname_langIndices[7];
  } else {
    m_ClientPrefix = "";
    m_clientFileString = "";
    m_name_lang[0] = "";
    m_name_lang[1] = "";
    m_name_lang[2] = "";
    m_name_lang[3] = "";
    m_name_lang[4] = "";
    m_name_lang[5] = "";
    m_name_lang[6] = "";
    m_name_lang[7] = "";
  }

  return true;
}
