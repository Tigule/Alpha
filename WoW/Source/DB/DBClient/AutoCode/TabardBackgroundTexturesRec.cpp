#include "TabardBackgroundTexturesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR TabardBackgroundTexturesRec::GetFilename() {
  return "DBFilesClient\\TabardBackgroundTextures.dbc";
}

TabardBackgroundTexturesRec::TabardBackgroundTexturesRec() {
}

TabardBackgroundTexturesRec::~TabardBackgroundTexturesRec() {
}

bool TabardBackgroundTexturesRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempTorsoTextureIndices[2];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempTorsoTextureIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempTorsoTextureIndices[1]) == 0);

  if (error) {
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
