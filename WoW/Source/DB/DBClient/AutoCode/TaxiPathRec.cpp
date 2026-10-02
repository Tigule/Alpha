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
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_FromTaxiNode) == 0);
  error |= (SFileReadTyped(f, &m_ToTaxiNode) == 0);
  error |= (SFileReadTyped(f, &m_Cost) == 0);

  if (error) {
    ConsoleWrite("Error reading TaxiPathRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
