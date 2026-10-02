#include "GroundEffectDoodadRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR GroundEffectDoodadRec::GetFilename() {
  return "DBFilesClient\\GroundEffectDoodad.dbc";
}

GroundEffectDoodadRec::GroundEffectDoodadRec() {
}

GroundEffectDoodadRec::~GroundEffectDoodadRec() {
}

bool GroundEffectDoodadRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempdoodadpathIndices[1];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_doodadIdTag) == 0);
  error |= (SFileReadTyped(f, &tempdoodadpathIndices[0]) == 0);

  if (error) {
    ConsoleWrite("Error reading GroundEffectDoodadRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_doodadpath = stringBuffer + tempdoodadpathIndices[0];
  } else {
    m_doodadpath = "";
  }

  return true;
}
