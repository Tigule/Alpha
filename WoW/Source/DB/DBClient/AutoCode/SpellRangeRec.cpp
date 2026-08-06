#include "SpellRangeRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR SpellRangeRec::GetFilename() {
  return "DBFilesClient\\SpellRange.dbc";
}

SpellRangeRec::SpellRangeRec() {
}

SpellRangeRec::~SpellRangeRec() {
}

bool SpellRangeRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempdisplayName_langIndices[8];
  UINT tempdisplayNameShort_langIndices[8];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_rangeMin, sizeof(m_rangeMin), 0, 0, 0) && result;
  result = SFile::Read(f, &m_rangeMax, sizeof(m_rangeMax), 0, 0, 0) && result;
  result = SFile::Read(f, &m_flags, sizeof(m_flags), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[0], sizeof(tempdisplayName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[1], sizeof(tempdisplayName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[2], sizeof(tempdisplayName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[3], sizeof(tempdisplayName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[4], sizeof(tempdisplayName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[5], sizeof(tempdisplayName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[6], sizeof(tempdisplayName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[7], sizeof(tempdisplayName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_displayName_flag, sizeof(m_displayName_flag), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayNameShort_langIndices[0], sizeof(tempdisplayNameShort_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayNameShort_langIndices[1], sizeof(tempdisplayNameShort_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayNameShort_langIndices[2], sizeof(tempdisplayNameShort_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayNameShort_langIndices[3], sizeof(tempdisplayNameShort_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayNameShort_langIndices[4], sizeof(tempdisplayNameShort_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayNameShort_langIndices[5], sizeof(tempdisplayNameShort_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayNameShort_langIndices[6], sizeof(tempdisplayNameShort_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayNameShort_langIndices[7], sizeof(tempdisplayNameShort_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_displayNameShort_flag, sizeof(m_displayNameShort_flag), 0, 0, 0) && result;

  if (!result) {
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
