#include "LockTypeRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR LockTypeRec::GetFilename() {
  return "DBFilesClient\\LockType.dbc";
}

LockTypeRec::LockTypeRec() {
}

LockTypeRec::~LockTypeRec() {
}

bool LockTypeRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempname_langIndices[8];
  UINT tempresourceName_langIndices[8];
  UINT tempverb_langIndices[8];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[0], sizeof(tempname_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[1], sizeof(tempname_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[2], sizeof(tempname_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[3], sizeof(tempname_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[4], sizeof(tempname_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[5], sizeof(tempname_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[6], sizeof(tempname_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempname_langIndices[7], sizeof(tempname_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_name_flag, sizeof(m_name_flag), 0, 0, 0) && result;
  result = SFile::Read(f, &tempresourceName_langIndices[0], sizeof(tempresourceName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempresourceName_langIndices[1], sizeof(tempresourceName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempresourceName_langIndices[2], sizeof(tempresourceName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempresourceName_langIndices[3], sizeof(tempresourceName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempresourceName_langIndices[4], sizeof(tempresourceName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempresourceName_langIndices[5], sizeof(tempresourceName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempresourceName_langIndices[6], sizeof(tempresourceName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempresourceName_langIndices[7], sizeof(tempresourceName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_resourceName_flag, sizeof(m_resourceName_flag), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverb_langIndices[0], sizeof(tempverb_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverb_langIndices[1], sizeof(tempverb_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverb_langIndices[2], sizeof(tempverb_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverb_langIndices[3], sizeof(tempverb_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverb_langIndices[4], sizeof(tempverb_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverb_langIndices[5], sizeof(tempverb_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverb_langIndices[6], sizeof(tempverb_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverb_langIndices[7], sizeof(tempverb_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_verb_flag, sizeof(m_verb_flag), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading LockTypeRec", DEFAULT_COLOR);
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
    m_resourceName_lang[0] = stringBuffer + tempresourceName_langIndices[0];
    m_resourceName_lang[1] = stringBuffer + tempresourceName_langIndices[1];
    m_resourceName_lang[2] = stringBuffer + tempresourceName_langIndices[2];
    m_resourceName_lang[3] = stringBuffer + tempresourceName_langIndices[3];
    m_resourceName_lang[4] = stringBuffer + tempresourceName_langIndices[4];
    m_resourceName_lang[5] = stringBuffer + tempresourceName_langIndices[5];
    m_resourceName_lang[6] = stringBuffer + tempresourceName_langIndices[6];
    m_resourceName_lang[7] = stringBuffer + tempresourceName_langIndices[7];
    m_verb_lang[0] = stringBuffer + tempverb_langIndices[0];
    m_verb_lang[1] = stringBuffer + tempverb_langIndices[1];
    m_verb_lang[2] = stringBuffer + tempverb_langIndices[2];
    m_verb_lang[3] = stringBuffer + tempverb_langIndices[3];
    m_verb_lang[4] = stringBuffer + tempverb_langIndices[4];
    m_verb_lang[5] = stringBuffer + tempverb_langIndices[5];
    m_verb_lang[6] = stringBuffer + tempverb_langIndices[6];
    m_verb_lang[7] = stringBuffer + tempverb_langIndices[7];
  } else {
    m_name_lang[0] = "";
    m_name_lang[1] = "";
    m_name_lang[2] = "";
    m_name_lang[3] = "";
    m_name_lang[4] = "";
    m_name_lang[5] = "";
    m_name_lang[6] = "";
    m_name_lang[7] = "";
    m_resourceName_lang[0] = "";
    m_resourceName_lang[1] = "";
    m_resourceName_lang[2] = "";
    m_resourceName_lang[3] = "";
    m_resourceName_lang[4] = "";
    m_resourceName_lang[5] = "";
    m_resourceName_lang[6] = "";
    m_resourceName_lang[7] = "";
    m_verb_lang[0] = "";
    m_verb_lang[1] = "";
    m_verb_lang[2] = "";
    m_verb_lang[3] = "";
    m_verb_lang[4] = "";
    m_verb_lang[5] = "";
    m_verb_lang[6] = "";
    m_verb_lang[7] = "";
  }

  return true;
}
