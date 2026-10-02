#include "SkillLineRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SkillLineRec::GetFilename() {
  return "DBFilesClient\\SkillLine.dbc";
}

SkillLineRec::SkillLineRec() {
}

SkillLineRec::~SkillLineRec() {
}

bool SkillLineRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempdisplayName_langIndices[8];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_raceMask) == 0);
  error |= (SFileReadTyped(f, &m_classMask) == 0);
  error |= (SFileReadTyped(f, &m_excludeRace) == 0);
  error |= (SFileReadTyped(f, &m_excludeClass) == 0);
  error |= (SFileReadTyped(f, &m_categoryID) == 0);
  error |= (SFileReadTyped(f, &m_skillType) == 0);
  error |= (SFileReadTyped(f, &m_minCharLevel) == 0);
  error |= (SFileReadTyped(f, &m_maxRank) == 0);
  error |= (SFileReadTyped(f, &m_abandonable) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempdisplayName_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_displayName_flag) == 0);

  if (error) {
    ConsoleWrite("Error reading SkillLineRec", DEFAULT_COLOR);
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
  } else {
    m_displayName_lang[0] = "";
    m_displayName_lang[1] = "";
    m_displayName_lang[2] = "";
    m_displayName_lang[3] = "";
    m_displayName_lang[4] = "";
    m_displayName_lang[5] = "";
    m_displayName_lang[6] = "";
    m_displayName_lang[7] = "";
  }

  return true;
}
