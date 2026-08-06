#include "ResistancesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR ResistancesRec::GetFilename() {
  return "DBFilesClient\\Resistances.dbc";
}

ResistancesRec::ResistancesRec() {
}

ResistancesRec::~ResistancesRec() {
}

bool ResistancesRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempname_langIndices[8];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Flags, sizeof(m_Flags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_FizzleSoundID, sizeof(m_FizzleSoundID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[0], sizeof(tempname_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[1], sizeof(tempname_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[2], sizeof(tempname_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[3], sizeof(tempname_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[4], sizeof(tempname_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[5], sizeof(tempname_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[6], sizeof(tempname_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[7], sizeof(tempname_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_name_flag, sizeof(m_name_flag), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading ResistancesRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_name_lang[0] = stringBuffer + tempname_langIndices[0];
    m_name_lang[1] = stringBuffer + tempname_langIndices[1];
    m_name_lang[2] = stringBuffer + tempname_langIndices[2];
    m_name_lang[3] = stringBuffer + tempname_langIndices[3];
    m_name_lang[4] = stringBuffer + tempname_langIndices[4];
    m_name_lang[5] = stringBuffer + tempname_langIndices[5];
    m_name_lang[6] = stringBuffer + tempname_langIndices[6];
    m_name_lang[7] = stringBuffer + tempname_langIndices[7];
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
