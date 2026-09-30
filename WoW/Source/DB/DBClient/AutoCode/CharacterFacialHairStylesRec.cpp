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

  if (!SFileReadTyped(f, &m_RaceID) ||
      !SFileReadTyped(f, &m_SexID) ||
      !SFileReadTyped(f, &m_VariationID) ||
      !SFileReadTyped(f, &m_BeardGeoset) ||
      !SFileReadTyped(f, &m_MoustacheGeoset) ||
      !SFileReadTyped(f, &m_SideburnGeoset)) {
    ConsoleWrite("Error reading CharacterFacialHairStylesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
