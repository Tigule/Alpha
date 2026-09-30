#include "ChrRacesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR ChrRacesRec::GetFilename() {
  return "DBFilesClient\\ChrRaces.dbc";
}

ChrRacesRec::ChrRacesRec() {
}

ChrRacesRec::~ChrRacesRec() {
}

bool ChrRacesRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempname_langIndices[NUM_LOCALES];
  UINT tempclientFileStringIndices[1];
  UINT tempClientPrefixIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_flags) ||
      !SFileReadTyped(f, &m_factionID) ||
      !SFileReadTyped(f, &m_MaleDisplayId) ||
      !SFileReadTyped(f, &m_FemaleDisplayId) ||
      !SFileReadTyped(f, &tempClientPrefixIndices[0]) ||
      !SFileReadTyped(f, &m_MountScale) ||
      !SFileReadTyped(f, &m_BaseLanguage) ||
      !SFileReadTyped(f, &m_creatureType) ||
      !SFileReadTyped(f, &m_LoginEffectSpellID) ||
      !SFileReadTyped(f, &m_CombatStunSpellID) ||
      !SFileReadTyped(f, &m_ResSicknessSpellID) ||
      !SFileReadTyped(f, &m_SplashSoundID) ||
      !SFileReadTyped(f, &m_startingTaxiNodes) ||
      !SFileReadTyped(f, &tempclientFileStringIndices[0]) ||
      !SFileReadTyped(f, &m_cinematicSequenceID) ||
      !SFileReadTyped(f, &tempname_langIndices[0]) ||
      !SFileReadTyped(f, &tempname_langIndices[1]) ||
      !SFileReadTyped(f, &tempname_langIndices[2]) ||
      !SFileReadTyped(f, &tempname_langIndices[3]) ||
      !SFileReadTyped(f, &tempname_langIndices[4]) ||
      !SFileReadTyped(f, &tempname_langIndices[5]) ||
      !SFileReadTyped(f, &tempname_langIndices[6]) ||
      !SFileReadTyped(f, &tempname_langIndices[7]) ||
      !SFileReadTyped(f, &m_name_flag)) {
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
