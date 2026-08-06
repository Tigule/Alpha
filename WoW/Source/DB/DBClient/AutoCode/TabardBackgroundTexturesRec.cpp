#include "TabardBackgroundTexturesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR TabardBackgroundTexturesRec::GetFilename() {
  return "DBFilesClient\\TabardBackgroundTextures.dbc";
}

TabardBackgroundTexturesRec::TabardBackgroundTexturesRec() {
}

TabardBackgroundTexturesRec::~TabardBackgroundTexturesRec() {
}

bool TabardBackgroundTexturesRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempTorsoTextureIndices[2];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempTorsoTextureIndices[0], sizeof(tempTorsoTextureIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempTorsoTextureIndices[1], sizeof(tempTorsoTextureIndices[1]), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading TabardBackgroundTexturesRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_TorsoTexture[0] = stringBuffer + tempTorsoTextureIndices[0];
    m_TorsoTexture[1] = stringBuffer + tempTorsoTextureIndices[1];
  } else {
    m_TorsoTexture[0] = "";
    m_TorsoTexture[1] = "";
  }

  return true;
}
