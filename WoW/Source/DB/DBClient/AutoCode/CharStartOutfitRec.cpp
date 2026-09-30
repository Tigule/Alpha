#include "CharStartOutfitRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CharStartOutfitRec::GetFilename() {
  return "DBFilesClient\\CharStartOutfit.dbc";
}

CharStartOutfitRec::CharStartOutfitRec() {
}

CharStartOutfitRec::~CharStartOutfitRec() {
}

bool CharStartOutfitRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFile::Read(f, &m_raceID, sizeof(m_raceID), 0, 0, 0) ||
      !SFile::Read(f, &m_classID, sizeof(m_classID), 0, 0, 0) ||
      !SFile::Read(f, &m_sexID, sizeof(m_sexID), 0, 0, 0) ||
      !SFile::Read(f, &m_outfitID, sizeof(m_outfitID), 0, 0, 0) ||
      !SFile::Read(f, &m_ItemID[0], sizeof(m_ItemID), 0, 0, 0) ||
      !SFile::Read(f, &m_DisplayItemID[0], sizeof(m_DisplayItemID), 0, 0, 0) ||
      !SFile::Read(f, &m_InventoryType[0], sizeof(m_InventoryType), 0, 0, 0)) {
    ConsoleWrite("Error reading CharStartOutfitRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
