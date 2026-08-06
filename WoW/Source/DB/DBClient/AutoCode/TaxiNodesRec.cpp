#include "TaxiNodesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

LPCSTR TaxiNodesRec::GetFilename() {
  return "DBFilesClient\\TaxiNodes.dbc";
}

TaxiNodesRec::TaxiNodesRec() {
}

TaxiNodesRec::~TaxiNodesRec() {
}

bool TaxiNodesRec::Read(SFile *f, LPCSTR stringBuffer) {
  bool result = true;
  UINT tempName_langIndices[8];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_ContinentID, sizeof(m_ContinentID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_X, sizeof(m_X), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Y, sizeof(m_Y), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Z, sizeof(m_Z), 0, 0, 0) && result;
  result = SFile::Read(f, &tempName_langIndices[0], sizeof(tempName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempName_langIndices[1], sizeof(tempName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempName_langIndices[2], sizeof(tempName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempName_langIndices[3], sizeof(tempName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempName_langIndices[4], sizeof(tempName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempName_langIndices[5], sizeof(tempName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempName_langIndices[6], sizeof(tempName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempName_langIndices[7], sizeof(tempName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Name_flag, sizeof(m_Name_flag), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading TaxiNodesRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_Name_lang[0] = stringBuffer + tempName_langIndices[0];
    m_Name_lang[1] = stringBuffer + tempName_langIndices[1];
    m_Name_lang[2] = stringBuffer + tempName_langIndices[2];
    m_Name_lang[3] = stringBuffer + tempName_langIndices[3];
    m_Name_lang[4] = stringBuffer + tempName_langIndices[4];
    m_Name_lang[5] = stringBuffer + tempName_langIndices[5];
    m_Name_lang[6] = stringBuffer + tempName_langIndices[6];
    m_Name_lang[7] = stringBuffer + tempName_langIndices[7];
  } else {
    m_Name_lang[0] = "";
    m_Name_lang[1] = "";
    m_Name_lang[2] = "";
    m_Name_lang[3] = "";
    m_Name_lang[4] = "";
    m_Name_lang[5] = "";
    m_Name_lang[6] = "";
    m_Name_lang[7] = "";
  }

  return true;
}
