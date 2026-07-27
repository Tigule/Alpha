#include "ItemVisualEffectsRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *ItemVisualEffectsRec::GetFilename() {
  return "DBFilesClient\\ItemVisualEffects.dbc";
}

ItemVisualEffectsRec::ItemVisualEffectsRec() {
}

ItemVisualEffectsRec::~ItemVisualEffectsRec() {
}

bool ItemVisualEffectsRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempModelIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempModelIndices[0], sizeof(tempModelIndices[0]), 0, 0, 0) && result;

  if (!result) {
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
