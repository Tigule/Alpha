#include "ItemVisualEffectsRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR ItemVisualEffectsRec::GetFilename() {
  return "DBFilesClient\\ItemVisualEffects.dbc";
}

ItemVisualEffectsRec::ItemVisualEffectsRec() {
}

ItemVisualEffectsRec::~ItemVisualEffectsRec() {
}

bool ItemVisualEffectsRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempModelIndices[1];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempModelIndices[0]) == 0);

  if (error) {
    ConsoleWrite("Error reading ItemVisualEffectsRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_Model = &stringBuffer[tempModelIndices[0]];
  } else {
    m_Model = "";
  }

  return true;
}
