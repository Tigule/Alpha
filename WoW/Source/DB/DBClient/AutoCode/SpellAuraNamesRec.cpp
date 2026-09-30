#include "SpellAuraNamesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellAuraNamesRec::GetFilename() {
  return "DBFilesClient\\SpellAuraNames.dbc";
}

SpellAuraNamesRec::SpellAuraNamesRec() {
}

SpellAuraNamesRec::~SpellAuraNamesRec() {
}

bool SpellAuraNamesRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempname_langIndices[8];
  UINT tempglobalstrings_tagIndices[1];

  if (!SFileReadTyped(f, &m_EnumID) ||
      !SFileReadTyped(f, &m_specialMiscValue) ||
      !SFileReadTyped(f, &tempglobalstrings_tagIndices[0]) ||
      !SFileReadTyped(f, &tempname_langIndices[0]) ||
      !SFileReadTyped(f, &tempname_langIndices[1]) ||
      !SFileReadTyped(f, &tempname_langIndices[2]) ||
      !SFileReadTyped(f, &tempname_langIndices[3]) ||
      !SFileReadTyped(f, &tempname_langIndices[4]) ||
      !SFileReadTyped(f, &tempname_langIndices[5]) ||
      !SFileReadTyped(f, &tempname_langIndices[6]) ||
      !SFileReadTyped(f, &tempname_langIndices[7]) ||
      !SFileReadTyped(f, &m_name_flag)) {
    ConsoleWrite("Error reading SpellAuraNamesRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_globalstrings_tag = stringBuffer + tempglobalstrings_tagIndices[0];
    m_name_lang[0] = stringBuffer + tempname_langIndices[0];
    m_name_lang[1] = stringBuffer + tempname_langIndices[1];
    m_name_lang[2] = stringBuffer + tempname_langIndices[2];
    m_name_lang[3] = stringBuffer + tempname_langIndices[3];
    m_name_lang[4] = stringBuffer + tempname_langIndices[4];
    m_name_lang[5] = stringBuffer + tempname_langIndices[5];
    m_name_lang[6] = stringBuffer + tempname_langIndices[6];
    m_name_lang[7] = stringBuffer + tempname_langIndices[7];
  } else {
    m_globalstrings_tag = "";
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
