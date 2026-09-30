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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempInfoName_langIndices[0]) ||
      !SFileReadTyped(f, &tempInfoName_langIndices[1]) ||
      !SFileReadTyped(f, &tempInfoName_langIndices[2]) ||
      !SFileReadTyped(f, &tempInfoName_langIndices[3]) ||
      !SFileReadTyped(f, &tempInfoName_langIndices[4]) ||
      !SFileReadTyped(f, &tempInfoName_langIndices[5]) ||
      !SFileReadTyped(f, &tempInfoName_langIndices[6]) ||
      !SFileReadTyped(f, &tempInfoName_langIndices[7]) ||
      !SFileReadTyped(f, &m_InfoName_flag)) {
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
