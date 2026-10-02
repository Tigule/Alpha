#include "EmotesTextDataRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR EmotesTextDataRec::GetFilename() {
  return "DBFilesClient\\EmotesTextData.dbc";
}

EmotesTextDataRec::EmotesTextDataRec() {
}

EmotesTextDataRec::~EmotesTextDataRec() {
}

bool EmotesTextDataRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT temptext_langIndices[NUM_LOCALES];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &temptext_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &temptext_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &temptext_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &temptext_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &temptext_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &temptext_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &temptext_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &temptext_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_text_flag) == 0);

  if (error) {
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
