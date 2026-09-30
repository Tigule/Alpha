#include "SpellRangeRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SpellRangeRec::GetFilename() {
  return "DBFilesClient\\SpellRange.dbc";
}

SpellRangeRec::SpellRangeRec() {
}

SpellRangeRec::~SpellRangeRec() {
}

bool SpellRangeRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempdisplayName_langIndices[8];
  UINT tempdisplayNameShort_langIndices[8];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_rangeMin) ||
      !SFileReadTyped(f, &m_rangeMax) ||
      !SFileReadTyped(f, &m_flags) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[0]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[1]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[2]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[3]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[4]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[5]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[6]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[7]) ||
      !SFileReadTyped(f, &m_displayName_flag) ||
      !SFileReadTyped(f, &tempdisplayNameShort_langIndices[0]) ||
      !SFileReadTyped(f, &tempdisplayNameShort_langIndices[1]) ||
      !SFileReadTyped(f, &tempdisplayNameShort_langIndices[2]) ||
      !SFileReadTyped(f, &tempdisplayNameShort_langIndices[3]) ||
      !SFileReadTyped(f, &tempdisplayNameShort_langIndices[4]) ||
      !SFileReadTyped(f, &tempdisplayNameShort_langIndices[5]) ||
      !SFileReadTyped(f, &tempdisplayNameShort_langIndices[6]) ||
      !SFileReadTyped(f, &tempdisplayNameShort_langIndices[7]) ||
      !SFileReadTyped(f, &m_displayNameShort_flag)) {
    ConsoleWrite("Error reading SpellRangeRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_displayName_lang[0] = stringBuffer + tempdisplayName_langIndices[0];
    m_displayName_lang[1] = stringBuffer + tempdisplayName_langIndices[1];
    m_displayName_lang[2] = stringBuffer + tempdisplayName_langIndices[2];
    m_displayName_lang[3] = stringBuffer + tempdisplayName_langIndices[3];
    m_displayName_lang[4] = stringBuffer + tempdisplayName_langIndices[4];
    m_displayName_lang[5] = stringBuffer + tempdisplayName_langIndices[5];
    m_displayName_lang[6] = stringBuffer + tempdisplayName_langIndices[6];
    m_displayName_lang[7] = stringBuffer + tempdisplayName_langIndices[7];
    m_displayNameShort_lang[0] = stringBuffer + tempdisplayNameShort_langIndices[0];
    m_displayNameShort_lang[1] = stringBuffer + tempdisplayNameShort_langIndices[1];
    m_displayNameShort_lang[2] = stringBuffer + tempdisplayNameShort_langIndices[2];
    m_displayNameShort_lang[3] = stringBuffer + tempdisplayNameShort_langIndices[3];
    m_displayNameShort_lang[4] = stringBuffer + tempdisplayNameShort_langIndices[4];
    m_displayNameShort_lang[5] = stringBuffer + tempdisplayNameShort_langIndices[5];
    m_displayNameShort_lang[6] = stringBuffer + tempdisplayNameShort_langIndices[6];
    m_displayNameShort_lang[7] = stringBuffer + tempdisplayNameShort_langIndices[7];
  } else {
    m_displayName_lang[0] = "";
    m_displayName_lang[1] = "";
    m_displayName_lang[2] = "";
    m_displayName_lang[3] = "";
    m_displayName_lang[4] = "";
    m_displayName_lang[5] = "";
    m_displayName_lang[6] = "";
    m_displayName_lang[7] = "";
    m_displayNameShort_lang[0] = "";
    m_displayNameShort_lang[1] = "";
    m_displayNameShort_lang[2] = "";
    m_displayNameShort_lang[3] = "";
    m_displayNameShort_lang[4] = "";
    m_displayNameShort_lang[5] = "";
    m_displayNameShort_lang[6] = "";
    m_displayNameShort_lang[7] = "";
  }

  return true;
}
