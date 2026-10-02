#include "FootprintTexturesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR FootprintTexturesRec::GetFilename() {
  return "DBFilesClient\\FootprintTextures.dbc";
}

FootprintTexturesRec::FootprintTexturesRec() {
}

FootprintTexturesRec::~FootprintTexturesRec() {
}

bool FootprintTexturesRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempFootstepFilenameIndices[1];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempFootstepFilenameIndices[0]) == 0);

  if (error) {
    ConsoleWrite("Error reading FootprintTexturesRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_FootstepFilename = stringBuffer + tempFootstepFilenameIndices[0];
  } else {
    m_FootstepFilename = "";
  }

  return true;
}
