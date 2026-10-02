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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_RaceID) == 0);
  error |= (SFileReadTyped(f, &m_SexID) == 0);
  error |= (SFileReadTyped(f, &m_SectionID) == 0);
  error |= (SFileReadTyped(f, &m_VariationID) == 0);
  error |= (SFileReadTyped(f, &m_ColorID) == 0);
  error |= (SFileReadTyped(f, &m_IsNPC) == 0);
  error |= (SFileReadTyped(f, &tempTextureNameIndices[0]) == 0);

  if (error) {
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
