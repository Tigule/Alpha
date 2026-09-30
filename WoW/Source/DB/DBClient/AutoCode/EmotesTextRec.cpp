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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempnameIndices[0]) ||
      !SFileReadTyped(f, &m_emoteID) ||
      !SFile::Read(f, m_emoteText, sizeof(m_emoteText), 0, 0, 0)) {
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
