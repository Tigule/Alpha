#include "EmoteAnimsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *EmoteAnimsRec::GetFilename() {
  return "DBFilesClient\\EmoteAnims.dbc";
}

EmoteAnimsRec::EmoteAnimsRec() {
}

EmoteAnimsRec::~EmoteAnimsRec() {
}

bool EmoteAnimsRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempAnimNameIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ProcessedAnimIndex, sizeof(m_ProcessedAnimIndex), 0, 0, 0) && result;
  result = SFile::Read(f, tempAnimNameIndices, sizeof(tempAnimNameIndices), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading EmoteAnimsRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_AnimName = &stringBuffer[tempAnimNameIndices[0]];
  } else {
    m_AnimName = "";
  }

  return true;
}
