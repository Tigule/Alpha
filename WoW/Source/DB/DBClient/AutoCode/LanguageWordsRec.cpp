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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_languageID) == 0);
  error |= (SFileReadTyped(f, &tempwordIndices[0]) == 0);

  if (error) {
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
