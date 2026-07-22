#include "EmotesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall EmotesRec::GetFilename() {
  return "DBFilesClient\\Emotes.dbc";
}

EmotesRec::EmotesRec() {
}

EmotesRec::~EmotesRec() {
}

bool EmotesRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_EmoteAnimID, sizeof(m_EmoteAnimID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_EmoteFlags, sizeof(m_EmoteFlags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_EmoteSpecProc, sizeof(m_EmoteSpecProc), 0, 0, 0) && result;
  result = SFile::Read(f, &m_EmoteSpecProcParam, sizeof(m_EmoteSpecProcParam), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading EmotesRec", DEFAULT_COLOR);
  }

  return result;
}
