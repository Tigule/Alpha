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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_doodadIdTag) ||
      !SFileReadTyped(f, &tempdoodadpathIndices[0])) {
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
