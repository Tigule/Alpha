#include "LanguageWordsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR LanguageWordsRec::GetFilename() {
  return "DBFilesClient\\LanguageWords.dbc";
}

LanguageWordsRec::LanguageWordsRec() {
}

LanguageWordsRec::~LanguageWordsRec() {
}

bool LanguageWordsRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempwordIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_languageID) ||
      !SFileReadTyped(f, &tempwordIndices[0])) {
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
