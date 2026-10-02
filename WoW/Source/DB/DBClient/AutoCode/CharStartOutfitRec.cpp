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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_raceID) == 0);
  error |= (SFileReadTyped(f, &m_classID) == 0);
  error |= (SFileReadTyped(f, &m_sexID) == 0);
  error |= (SFileReadTyped(f, &m_outfitID) == 0);
  error |= (SFileReadTyped(f, &m_ItemID) == 0);
  error |= (SFileReadTyped(f, &m_DisplayItemID) == 0);
  error |= (SFileReadTyped(f, &m_InventoryType) == 0);

  if (error) {
    ConsoleWrite("Error reading CharStartOutfitRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
