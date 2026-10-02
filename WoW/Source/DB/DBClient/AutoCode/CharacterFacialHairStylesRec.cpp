#include "CharacterFacialHairStylesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CharacterFacialHairStylesRec::GetFilename() {
  return "DBFilesClient\\CharacterFacialHairStyles.dbc";
}

CharacterFacialHairStylesRec::CharacterFacialHairStylesRec() {
}

CharacterFacialHairStylesRec::~CharacterFacialHairStylesRec() {
}

bool CharacterFacialHairStylesRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_RaceID) == 0);
  error |= (SFileReadTyped(f, &m_SexID) == 0);
  error |= (SFileReadTyped(f, &m_VariationID) == 0);
  error |= (SFileReadTyped(f, &m_BeardGeoset) == 0);
  error |= (SFileReadTyped(f, &m_MoustacheGeoset) == 0);
  error |= (SFileReadTyped(f, &m_SideburnGeoset) == 0);

  if (error) {
    ConsoleWrite("Error reading CharacterFacialHairStylesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
