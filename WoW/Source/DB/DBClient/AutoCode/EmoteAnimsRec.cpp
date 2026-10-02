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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_ProcessedAnimIndex) == 0);
  error |= (SFileReadTyped(f, &tempAnimNameIndices) == 0);

  if (error) {
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
