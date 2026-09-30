#include "TabardEmblemTexturesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR TabardEmblemTexturesRec::GetFilename() {
  return "DBFilesClient\\TabardEmblemTextures.dbc";
}

TabardEmblemTexturesRec::TabardEmblemTexturesRec() {
}

TabardEmblemTexturesRec::~TabardEmblemTexturesRec() {
}

bool TabardEmblemTexturesRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempTorsoTextureIndices[2];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempTorsoTextureIndices[0]) ||
      !SFileReadTyped(f, &tempTorsoTextureIndices[1])) {
    ConsoleWrite("Error reading TabardEmblemTexturesRec", DEFAULT_COLOR);
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
