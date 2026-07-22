#include "CharHairGeosetsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall CharHairGeosetsRec::GetFilename() {
  return "DBFilesClient\\CharHairGeosets.dbc";
}

CharHairGeosetsRec::CharHairGeosetsRec() {
}

CharHairGeosetsRec::~CharHairGeosetsRec() {
}

bool CharHairGeosetsRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_RaceID, sizeof(m_RaceID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SexID, sizeof(m_SexID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_VariationID, sizeof(m_VariationID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_GeosetID, sizeof(m_GeosetID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Showscalp, sizeof(m_Showscalp), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CharHairGeosetsRec", DEFAULT_COLOR);
  }

  return result;
}
