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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_RaceID) == 0);
  error |= (SFileReadTyped(f, &m_SexID) == 0);
  error |= (SFileReadTyped(f, &m_VariationID) == 0);
  error |= (SFileReadTyped(f, &m_GeosetID) == 0);
  error |= (SFileReadTyped(f, &m_Showscalp) == 0);

  if (error) {
    ConsoleWrite("Error reading CharHairGeosetsRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
