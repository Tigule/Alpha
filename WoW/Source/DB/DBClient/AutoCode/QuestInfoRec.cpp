#include "QuestInfoRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR QuestInfoRec::GetFilename() {
  return "DBFilesClient\\QuestInfo.dbc";
}

QuestInfoRec::QuestInfoRec() {
}

QuestInfoRec::~QuestInfoRec() {
}

bool QuestInfoRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempInfoName_langIndices[NUM_LOCALES];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempInfoName_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempInfoName_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempInfoName_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempInfoName_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempInfoName_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempInfoName_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempInfoName_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempInfoName_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_InfoName_flag) == 0);

  if (error) {
    ConsoleWrite("Error reading QuestInfoRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_InfoName_lang[0] = &stringBuffer[tempInfoName_langIndices[0]];
    m_InfoName_lang[1] = &stringBuffer[tempInfoName_langIndices[1]];
    m_InfoName_lang[2] = &stringBuffer[tempInfoName_langIndices[2]];
    m_InfoName_lang[3] = &stringBuffer[tempInfoName_langIndices[3]];
    m_InfoName_lang[4] = &stringBuffer[tempInfoName_langIndices[4]];
    m_InfoName_lang[5] = &stringBuffer[tempInfoName_langIndices[5]];
    m_InfoName_lang[6] = &stringBuffer[tempInfoName_langIndices[6]];
    m_InfoName_lang[7] = &stringBuffer[tempInfoName_langIndices[7]];
  } else {
    m_InfoName_lang[0] = "";
    m_InfoName_lang[1] = "";
    m_InfoName_lang[2] = "";
    m_InfoName_lang[3] = "";
    m_InfoName_lang[4] = "";
    m_InfoName_lang[5] = "";
    m_InfoName_lang[6] = "";
    m_InfoName_lang[7] = "";
  }

  return true;
}
