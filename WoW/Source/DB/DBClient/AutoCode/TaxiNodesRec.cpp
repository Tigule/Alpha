#include "TaxiNodesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR TaxiNodesRec::GetFilename() {
  return "DBFilesClient\\TaxiNodes.dbc";
}

TaxiNodesRec::TaxiNodesRec() {
}

TaxiNodesRec::~TaxiNodesRec() {
}

bool TaxiNodesRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempName_langIndices[8];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_ContinentID) ||
      !SFileReadTyped(f, &m_X) ||
      !SFileReadTyped(f, &m_Y) ||
      !SFileReadTyped(f, &m_Z) ||
      !SFileReadTyped(f, &tempName_langIndices[0]) ||
      !SFileReadTyped(f, &tempName_langIndices[1]) ||
      !SFileReadTyped(f, &tempName_langIndices[2]) ||
      !SFileReadTyped(f, &tempName_langIndices[3]) ||
      !SFileReadTyped(f, &tempName_langIndices[4]) ||
      !SFileReadTyped(f, &tempName_langIndices[5]) ||
      !SFileReadTyped(f, &tempName_langIndices[6]) ||
      !SFileReadTyped(f, &tempName_langIndices[7]) ||
      !SFileReadTyped(f, &m_Name_flag)) {
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
