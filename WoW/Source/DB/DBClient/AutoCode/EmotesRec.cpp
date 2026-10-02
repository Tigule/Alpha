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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_EmoteAnimID) == 0);
  error |= (SFileReadTyped(f, &m_EmoteFlags) == 0);
  error |= (SFileReadTyped(f, &m_EmoteSpecProc) == 0);
  error |= (SFileReadTyped(f, &m_EmoteSpecProcParam) == 0);

  if (error) {
    ConsoleWrite("Error reading EmotesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
