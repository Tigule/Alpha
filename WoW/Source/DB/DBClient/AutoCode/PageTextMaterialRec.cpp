#include "PageTextMaterialRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR PageTextMaterialRec::GetFilename() {
  return "DBFilesClient\\PageTextMaterial.dbc";
}

PageTextMaterialRec::PageTextMaterialRec() {
}

PageTextMaterialRec::~PageTextMaterialRec() {
}

bool PageTextMaterialRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempnameIndices[1];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempnameIndices[0]) == 0);

  if (error) {
    ConsoleWrite("Error reading PageTextMaterialRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_name = stringBuffer + tempnameIndices[0];
  } else {
    m_name = "";
  }

  return true;
}
