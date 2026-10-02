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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_DisplayRaceID) == 0);
  error |= (SFileReadTyped(f, &m_DisplaySexID) == 0);
  error |= (SFileReadTyped(f, &m_SkinID) == 0);
  error |= (SFileReadTyped(f, &m_FaceID) == 0);
  error |= (SFileReadTyped(f, &m_HairStyleID) == 0);
  error |= (SFileReadTyped(f, &m_HairColorID) == 0);
  error |= (SFileReadTyped(f, &m_FacialHairID) == 0);
  error |= (SFileReadTyped(f, &m_NPCItemDisplay) == 0);
  error |= (SFileReadTyped(f, &tempBakeNameIndices[0]) == 0);

  if (error) {
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
