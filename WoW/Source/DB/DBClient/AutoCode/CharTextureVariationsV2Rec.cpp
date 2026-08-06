#include "CharTextureVariationsV2Rec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR CharTextureVariationsV2Rec::GetFilename() {
  return "DBFilesClient\\CharTextureVariationsV2.dbc";
}

CharTextureVariationsV2Rec::CharTextureVariationsV2Rec() {
}

CharTextureVariationsV2Rec::~CharTextureVariationsV2Rec() {
}

bool CharTextureVariationsV2Rec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempTextureNameIndices[1];

  result = SFileReadTyped(f, &m_ID) && result;
  result = SFileReadTyped(f, &m_RaceID) && result;
  result = SFileReadTyped(f, &m_SexID) && result;
  result = SFileReadTyped(f, &m_SectionID) && result;
  result = SFileReadTyped(f, &m_VariationID) && result;
  result = SFileReadTyped(f, &m_ColorID) && result;
  result = SFileReadTyped(f, &m_IsNPC) && result;
  result = SFileReadTyped(f, &tempTextureNameIndices[0]) && result;

  if (!result) {
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
