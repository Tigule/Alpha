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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_ContinentID) == 0);
  error |= (SFileReadTyped(f, &m_X) == 0);
  error |= (SFileReadTyped(f, &m_Y) == 0);
  error |= (SFileReadTyped(f, &m_Z) == 0);
  error |= (SFileReadTyped(f, &tempName_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempName_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempName_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempName_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempName_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempName_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempName_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempName_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_Name_flag) == 0);

  if (error) {
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
