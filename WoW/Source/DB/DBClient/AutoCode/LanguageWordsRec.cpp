#include "LanguageWordsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *LanguageWordsRec::GetFilename() {
  return "DBFilesClient\\LanguageWords.dbc";
}

LanguageWordsRec::LanguageWordsRec() {
}

LanguageWordsRec::~LanguageWordsRec() {
}

bool LanguageWordsRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempwordIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_languageID, sizeof(m_languageID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempwordIndices[0], sizeof(tempwordIndices[0]), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading LanguageWordsRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_word = stringBuffer + tempwordIndices[0];
  } else {
    m_word = "";
  }

  return true;
}
