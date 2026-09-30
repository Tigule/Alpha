#include "CharTextureVariationsV2Rec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR CharTextureVariationsV2Rec::GetFilename() {
  return "DBFilesClient\\CharTextureVariationsV2.dbc";
}

CharTextureVariationsV2Rec::CharTextureVariationsV2Rec() {
}

CharTextureVariationsV2Rec::~CharTextureVariationsV2Rec() {
}

bool CharTextureVariationsV2Rec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempTextureNameIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_RaceID) ||
      !SFileReadTyped(f, &m_SexID) ||
      !SFileReadTyped(f, &m_SectionID) ||
      !SFileReadTyped(f, &m_VariationID) ||
      !SFileReadTyped(f, &m_ColorID) ||
      !SFileReadTyped(f, &m_IsNPC) ||
      !SFileReadTyped(f, &tempTextureNameIndices[0])) {
    ConsoleWrite("Error reading CharTextureVariationsV2Rec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_TextureName = &stringBuffer[tempTextureNameIndices[0]];
  } else {
    m_TextureName = "";
  }

  return true;
}
