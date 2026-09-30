#include "PaperDollItemFrameRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR PaperDollItemFrameRec::GetFilename() {
  return "DBFilesClient\\PaperDollItemFrame.dbc";
}

PaperDollItemFrameRec::PaperDollItemFrameRec() {
}

PaperDollItemFrameRec::~PaperDollItemFrameRec() {
}

bool PaperDollItemFrameRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempItemButtonNameIndices[1];
  UINT tempSlotIconIndices[1];

  if (!SFileReadTyped(f, &tempItemButtonNameIndices[0]) ||
      !SFileReadTyped(f, &tempSlotIconIndices[0]) ||
      !SFileReadTyped(f, &m_SlotNumber)) {
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
