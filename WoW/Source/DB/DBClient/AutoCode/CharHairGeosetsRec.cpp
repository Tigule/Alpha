#include "CharHairGeosetsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CharHairGeosetsRec::GetFilename() {
  return "DBFilesClient\\CharHairGeosets.dbc";
}

CharHairGeosetsRec::CharHairGeosetsRec() {
}

CharHairGeosetsRec::~CharHairGeosetsRec() {
}

bool CharHairGeosetsRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_RaceID) ||
      !SFileReadTyped(f, &m_SexID) ||
      !SFileReadTyped(f, &m_VariationID) ||
      !SFileReadTyped(f, &m_GeosetID) ||
      !SFileReadTyped(f, &m_Showscalp)) {
    ConsoleWrite("Error reading CharHairGeosetsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
