#include "SkillLineRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR SkillLineRec::GetFilename() {
  return "DBFilesClient\\SkillLine.dbc";
}

SkillLineRec::SkillLineRec() {
}

SkillLineRec::~SkillLineRec() {
}

bool SkillLineRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempdisplayName_langIndices[8];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_raceMask, sizeof(m_raceMask), 0, 0, 0) && result;
  result = SFile::Read(f, &m_classMask, sizeof(m_classMask), 0, 0, 0) && result;
  result = SFile::Read(f, &m_excludeRace, sizeof(m_excludeRace), 0, 0, 0) && result;
  result = SFile::Read(f, &m_excludeClass, sizeof(m_excludeClass), 0, 0, 0) && result;
  result = SFile::Read(f, &m_categoryID, sizeof(m_categoryID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_skillType, sizeof(m_skillType), 0, 0, 0) && result;
  result = SFile::Read(f, &m_minCharLevel, sizeof(m_minCharLevel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_maxRank, sizeof(m_maxRank), 0, 0, 0) && result;
  result = SFile::Read(f, &m_abandonable, sizeof(m_abandonable), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[0], sizeof(tempdisplayName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[1], sizeof(tempdisplayName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[2], sizeof(tempdisplayName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[3], sizeof(tempdisplayName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[4], sizeof(tempdisplayName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[5], sizeof(tempdisplayName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[6], sizeof(tempdisplayName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[7], sizeof(tempdisplayName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_displayName_flag, sizeof(m_displayName_flag), 0, 0, 0) && result;

  if (!result) {
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
