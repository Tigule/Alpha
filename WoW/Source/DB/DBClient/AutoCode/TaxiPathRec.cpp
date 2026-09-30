#include "TaxiPathRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR TaxiPathRec::GetFilename() {
  return "DBFilesClient\\TaxiPath.dbc";
}

TaxiPathRec::TaxiPathRec() {
}

TaxiPathRec::~TaxiPathRec() {
}

bool TaxiPathRec::Read(SFile *f, LPCSTR stringBuffer) {

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_FromTaxiNode) ||
      !SFileReadTyped(f, &m_ToTaxiNode) ||
      !SFileReadTyped(f, &m_Cost)) {
    ConsoleWrite("Error reading TaxiPathRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
