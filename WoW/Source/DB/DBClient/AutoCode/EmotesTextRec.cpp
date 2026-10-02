#include "EmotesTextRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR EmotesTextRec::GetFilename() {
  return "DBFilesClient\\EmotesText.dbc";
}

EmotesTextRec::EmotesTextRec() {
}

EmotesTextRec::~EmotesTextRec() {
}

bool EmotesTextRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempnameIndices[1];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempnameIndices[0]) == 0);
  error |= (SFileReadTyped(f, &m_emoteID) == 0);
  error |= (SFileReadTyped(f, &m_emoteText) == 0);

  if (error) {
    ConsoleWrite("Error reading EmotesTextRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_name = &stringBuffer[tempnameIndices[0]];
  } else {
    m_name = "";
  }

  return true;
}
