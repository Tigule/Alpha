#include "CharStartOutfitRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *CharStartOutfitRec::GetFilename() {
  return "DBFilesClient\\CharStartOutfit.dbc";
}

CharStartOutfitRec::CharStartOutfitRec() {
}

CharStartOutfitRec::~CharStartOutfitRec() {
}

bool CharStartOutfitRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_raceID, sizeof(m_raceID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_classID, sizeof(m_classID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_sexID, sizeof(m_sexID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_outfitID, sizeof(m_outfitID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ItemID[0], sizeof(m_ItemID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_DisplayItemID[0], sizeof(m_DisplayItemID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_InventoryType[0], sizeof(m_InventoryType), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading CharStartOutfitRec", DEFAULT_COLOR);
  }

  return result;
}
