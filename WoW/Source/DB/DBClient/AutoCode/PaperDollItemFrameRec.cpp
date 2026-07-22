#include "PaperDollItemFrameRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall PaperDollItemFrameRec::GetFilename() {
  return "DBFilesClient\\PaperDollItemFrame.dbc";
}

PaperDollItemFrameRec::PaperDollItemFrameRec() {
}

PaperDollItemFrameRec::~PaperDollItemFrameRec() {
}

bool PaperDollItemFrameRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempItemButtonNameIndices[1];
  unsigned int tempSlotIconIndices[1];

  result = SFile::Read(f, &tempItemButtonNameIndices[0], sizeof(tempItemButtonNameIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempSlotIconIndices[0], sizeof(tempSlotIconIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SlotNumber, sizeof(m_SlotNumber), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading PaperDollItemFrameRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_ItemButtonName = stringBuffer + tempItemButtonNameIndices[0];
    m_SlotIcon = stringBuffer + tempSlotIconIndices[0];
  } else {
    m_ItemButtonName = "";
    m_SlotIcon = "";
  }

  return true;
}
