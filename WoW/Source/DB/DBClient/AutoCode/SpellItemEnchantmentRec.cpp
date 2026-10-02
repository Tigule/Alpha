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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_effect) == 0);
  error |= (SFileReadTyped(f, &m_effectPointsMin) == 0);
  error |= (SFileReadTyped(f, &m_effectPointsMax) == 0);
  error |= (SFileReadTyped(f, &m_effectArg) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempname_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_name_flag) == 0);
  error |= (SFileReadTyped(f, &m_itemVisual) == 0);

  if (error) {
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
