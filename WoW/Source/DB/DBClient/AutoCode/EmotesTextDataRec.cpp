#include "EmotesTextDataRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *EmotesTextDataRec::GetFilename() {
  return "DBFilesClient\\EmotesTextData.dbc";
}

EmotesTextDataRec::EmotesTextDataRec() {
}

EmotesTextDataRec::~EmotesTextDataRec() {
}

bool EmotesTextDataRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int temptext_langIndices[NUM_LOCALES];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &temptext_langIndices[0], sizeof(temptext_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptext_langIndices[1], sizeof(temptext_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptext_langIndices[2], sizeof(temptext_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptext_langIndices[3], sizeof(temptext_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptext_langIndices[4], sizeof(temptext_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptext_langIndices[5], sizeof(temptext_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptext_langIndices[6], sizeof(temptext_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &temptext_langIndices[7], sizeof(temptext_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_text_flag, sizeof(m_text_flag), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading EmotesTextDataRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_text_lang[0] = &stringBuffer[temptext_langIndices[0]];
    m_text_lang[1] = &stringBuffer[temptext_langIndices[1]];
    m_text_lang[2] = &stringBuffer[temptext_langIndices[2]];
    m_text_lang[3] = &stringBuffer[temptext_langIndices[3]];
    m_text_lang[4] = &stringBuffer[temptext_langIndices[4]];
    m_text_lang[5] = &stringBuffer[temptext_langIndices[5]];
    m_text_lang[6] = &stringBuffer[temptext_langIndices[6]];
    m_text_lang[7] = &stringBuffer[temptext_langIndices[7]];
  } else {
    m_text_lang[0] = "";
    m_text_lang[1] = "";
    m_text_lang[2] = "";
    m_text_lang[3] = "";
    m_text_lang[4] = "";
    m_text_lang[5] = "";
    m_text_lang[6] = "";
    m_text_lang[7] = "";
  }

  return true;
}
