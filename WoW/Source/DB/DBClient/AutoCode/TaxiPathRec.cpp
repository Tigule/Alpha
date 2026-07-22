#include "TaxiPathRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall TaxiPathRec::GetFilename() {
  return "DBFilesClient\\TaxiPath.dbc";
}

TaxiPathRec::TaxiPathRec() {
}

TaxiPathRec::~TaxiPathRec() {
}

bool TaxiPathRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_FromTaxiNode, sizeof(m_FromTaxiNode), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ToTaxiNode, sizeof(m_ToTaxiNode), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Cost, sizeof(m_Cost), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading TaxiPathRec", DEFAULT_COLOR);
  }

  return result;
}
