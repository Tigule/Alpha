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
  int  error = 0;

  error |= (SFileReadTyped(f, &tempItemButtonNameIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempSlotIconIndices[0]) == 0);
  error |= (SFileReadTyped(f, &m_SlotNumber) == 0);

  if (error) {
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
