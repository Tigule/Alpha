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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_raceMask) ||
      !SFileReadTyped(f, &m_classMask) ||
      !SFileReadTyped(f, &m_excludeRace) ||
      !SFileReadTyped(f, &m_excludeClass) ||
      !SFileReadTyped(f, &m_categoryID) ||
      !SFileReadTyped(f, &m_skillType) ||
      !SFileReadTyped(f, &m_minCharLevel) ||
      !SFileReadTyped(f, &m_maxRank) ||
      !SFileReadTyped(f, &m_abandonable) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[0]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[1]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[2]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[3]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[4]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[5]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[6]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[7]) ||
      !SFileReadTyped(f, &m_displayName_flag)) {
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
