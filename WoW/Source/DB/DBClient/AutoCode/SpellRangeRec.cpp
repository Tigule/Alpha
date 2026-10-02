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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_rangeMin) == 0);
  error |= (SFileReadTyped(f, &m_rangeMax) == 0);
  error |= (SFileReadTyped(f, &m_flags) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_displayName_flag) == 0);
  error |= (SFileReadTyped(f, &tempdisplayNameShort_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayNameShort_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayNameShort_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayNameShort_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayNameShort_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayNameShort_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayNameShort_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayNameShort_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_displayNameShort_flag) == 0);

  if (error) {
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
