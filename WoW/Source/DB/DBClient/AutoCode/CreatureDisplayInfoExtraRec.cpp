#include "CreatureDisplayInfoExtraRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR CreatureDisplayInfoExtraRec::GetFilename() {
  return "DBFilesClient\\CreatureDisplayInfoExtra.dbc";
}

CreatureDisplayInfoExtraRec::CreatureDisplayInfoExtraRec() {
}

CreatureDisplayInfoExtraRec::~CreatureDisplayInfoExtraRec() {
}

bool CreatureDisplayInfoExtraRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempBakeNameIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_DisplayRaceID, sizeof(m_DisplayRaceID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_DisplaySexID, sizeof(m_DisplaySexID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SkinID, sizeof(m_SkinID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_FaceID, sizeof(m_FaceID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_HairStyleID, sizeof(m_HairStyleID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_HairColorID, sizeof(m_HairColorID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_FacialHairID, sizeof(m_FacialHairID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_NPCItemDisplay[0], sizeof(m_NPCItemDisplay), 0, 0, 0) && result;
  result = SFile::Read(f, &tempBakeNameIndices[0], sizeof(tempBakeNameIndices[0]), 0, 0, 0) && result;

  if (!result) {
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
