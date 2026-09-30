#include "EmoteAnimsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR EmoteAnimsRec::GetFilename() {
  return "DBFilesClient\\EmoteAnims.dbc";
}

EmoteAnimsRec::EmoteAnimsRec() {
}

EmoteAnimsRec::~EmoteAnimsRec() {
}

bool EmoteAnimsRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempAnimNameIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_ProcessedAnimIndex) ||
      !SFile::Read(f, tempAnimNameIndices, sizeof(tempAnimNameIndices), 0, 0, 0)) {
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
