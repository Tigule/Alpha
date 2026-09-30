#include "SpellItemEnchantmentRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellItemEnchantmentRec::GetFilename() {
  return "DBFilesClient\\SpellItemEnchantment.dbc";
}

SpellItemEnchantmentRec::SpellItemEnchantmentRec() {
}

SpellItemEnchantmentRec::~SpellItemEnchantmentRec() {
}

bool SpellItemEnchantmentRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempname_langIndices[NUM_LOCALES];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFile::Read(f, m_effect, sizeof(m_effect), 0, 0, 0) ||
      !SFile::Read(f, m_effectPointsMin, sizeof(m_effectPointsMin), 0, 0, 0) ||
      !SFile::Read(f, m_effectPointsMax, sizeof(m_effectPointsMax), 0, 0, 0) ||
      !SFile::Read(f, m_effectArg, sizeof(m_effectArg), 0, 0, 0) ||
      !SFileReadTyped(f, &tempname_langIndices[0]) ||
      !SFileReadTyped(f, &tempname_langIndices[1]) ||
      !SFileReadTyped(f, &tempname_langIndices[2]) ||
      !SFileReadTyped(f, &tempname_langIndices[3]) ||
      !SFileReadTyped(f, &tempname_langIndices[4]) ||
      !SFileReadTyped(f, &tempname_langIndices[5]) ||
      !SFileReadTyped(f, &tempname_langIndices[6]) ||
      !SFileReadTyped(f, &tempname_langIndices[7]) ||
      !SFileReadTyped(f, &m_name_flag) ||
      !SFileReadTyped(f, &m_itemVisual)) {
    ConsoleWrite("Error reading SpellItemEnchantmentRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_name_lang[0] = &stringBuffer[tempname_langIndices[0]];
    m_name_lang[1] = &stringBuffer[tempname_langIndices[1]];
    m_name_lang[2] = &stringBuffer[tempname_langIndices[2]];
    m_name_lang[3] = &stringBuffer[tempname_langIndices[3]];
    m_name_lang[4] = &stringBuffer[tempname_langIndices[4]];
    m_name_lang[5] = &stringBuffer[tempname_langIndices[5]];
    m_name_lang[6] = &stringBuffer[tempname_langIndices[6]];
    m_name_lang[7] = &stringBuffer[tempname_langIndices[7]];
  } else {
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
