#include "QuestInfoRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall QuestInfoRec::GetFilename() {
  return "DBFilesClient\\QuestInfo.dbc";
}

QuestInfoRec::QuestInfoRec() {
}

QuestInfoRec::~QuestInfoRec() {
}

bool QuestInfoRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempInfoName_langIndices[NUM_LOCALES];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempInfoName_langIndices[0], sizeof(tempInfoName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempInfoName_langIndices[1], sizeof(tempInfoName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempInfoName_langIndices[2], sizeof(tempInfoName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempInfoName_langIndices[3], sizeof(tempInfoName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempInfoName_langIndices[4], sizeof(tempInfoName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempInfoName_langIndices[5], sizeof(tempInfoName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempInfoName_langIndices[6], sizeof(tempInfoName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempInfoName_langIndices[7], sizeof(tempInfoName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_InfoName_flag, sizeof(m_InfoName_flag), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading QuestInfoRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_InfoName_lang[0] = &stringBuffer[tempInfoName_langIndices[0]];
    m_InfoName_lang[1] = &stringBuffer[tempInfoName_langIndices[1]];
    m_InfoName_lang[2] = &stringBuffer[tempInfoName_langIndices[2]];
    m_InfoName_lang[3] = &stringBuffer[tempInfoName_langIndices[3]];
    m_InfoName_lang[4] = &stringBuffer[tempInfoName_langIndices[4]];
    m_InfoName_lang[5] = &stringBuffer[tempInfoName_langIndices[5]];
    m_InfoName_lang[6] = &stringBuffer[tempInfoName_langIndices[6]];
    m_InfoName_lang[7] = &stringBuffer[tempInfoName_langIndices[7]];
  } else {
    m_InfoName_lang[0] = "";
    m_InfoName_lang[1] = "";
    m_InfoName_lang[2] = "";
    m_InfoName_lang[3] = "";
    m_InfoName_lang[4] = "";
    m_InfoName_lang[5] = "";
    m_InfoName_lang[6] = "";
    m_InfoName_lang[7] = "";
  }

  return true;
}
