#include "EmotesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR EmotesRec::GetFilename() {
  return "DBFilesClient\\Emotes.dbc";
}

EmotesRec::EmotesRec() {
}

EmotesRec::~EmotesRec() {
}

bool EmotesRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_EmoteAnimID) ||
      !SFileReadTyped(f, &m_EmoteFlags) ||
      !SFileReadTyped(f, &m_EmoteSpecProc) ||
      !SFileReadTyped(f, &m_EmoteSpecProcParam)) {
    ConsoleWrite("Error reading EmotesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
