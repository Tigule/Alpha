#include "CreatureDisplayInfoExtraRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CreatureDisplayInfoExtraRec::GetFilename() {
  return "DBFilesClient\\CreatureDisplayInfoExtra.dbc";
}

CreatureDisplayInfoExtraRec::CreatureDisplayInfoExtraRec() {
}

CreatureDisplayInfoExtraRec::~CreatureDisplayInfoExtraRec() {
}

bool CreatureDisplayInfoExtraRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempBakeNameIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_DisplayRaceID) ||
      !SFileReadTyped(f, &m_DisplaySexID) ||
      !SFileReadTyped(f, &m_SkinID) ||
      !SFileReadTyped(f, &m_FaceID) ||
      !SFileReadTyped(f, &m_HairStyleID) ||
      !SFileReadTyped(f, &m_HairColorID) ||
      !SFileReadTyped(f, &m_FacialHairID) ||
      !SFile::Read(f, &m_NPCItemDisplay[0], sizeof(m_NPCItemDisplay), 0, 0, 0) ||
      !SFileReadTyped(f, &tempBakeNameIndices[0])) {
    ConsoleWrite("Error reading CreatureDisplayInfoExtraRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_BakeName = stringBuffer + tempBakeNameIndices[0];
  } else {
    m_BakeName = "";
  }

  return true;
}
