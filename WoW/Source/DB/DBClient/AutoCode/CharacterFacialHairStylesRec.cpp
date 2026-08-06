#include "CharacterFacialHairStylesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR CharacterFacialHairStylesRec::GetFilename() {
  return "DBFilesClient\\CharacterFacialHairStyles.dbc";
}

CharacterFacialHairStylesRec::CharacterFacialHairStylesRec() {
}

CharacterFacialHairStylesRec::~CharacterFacialHairStylesRec() {
}

bool CharacterFacialHairStylesRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_RaceID, sizeof(m_RaceID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SexID, sizeof(m_SexID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_VariationID, sizeof(m_VariationID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_BeardGeoset, sizeof(m_BeardGeoset), 0, 0, 0) && result;
  result = SFile::Read(f, &m_MoustacheGeoset, sizeof(m_MoustacheGeoset), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SideburnGeoset, sizeof(m_SideburnGeoset), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CharacterFacialHairStylesRec", DEFAULT_COLOR);
  }

  return result;
}
