#include "QuestSortRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR QuestSortRec::GetFilename() {
  return "DBFilesClient\\QuestSort.dbc";
}

QuestSortRec::QuestSortRec() {
}

QuestSortRec::~QuestSortRec() {
}

bool QuestSortRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempSortName_langIndices[NUM_LOCALES];
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempSortName_langIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempSortName_langIndices[1]) == 0);
  error |= (SFileReadTyped(f, &tempSortName_langIndices[2]) == 0);
  error |= (SFileReadTyped(f, &tempSortName_langIndices[3]) == 0);
  error |= (SFileReadTyped(f, &tempSortName_langIndices[4]) == 0);
  error |= (SFileReadTyped(f, &tempSortName_langIndices[5]) == 0);
  error |= (SFileReadTyped(f, &tempSortName_langIndices[6]) == 0);
  error |= (SFileReadTyped(f, &tempSortName_langIndices[7]) == 0);
  error |= (SFileReadTyped(f, &m_SortName_flag) == 0);

  if (error) {
    ConsoleWrite("Error reading QuestSortRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_SortName_lang[0] = &stringBuffer[tempSortName_langIndices[0]];
    m_SortName_lang[1] = &stringBuffer[tempSortName_langIndices[1]];
    m_SortName_lang[2] = &stringBuffer[tempSortName_langIndices[2]];
    m_SortName_lang[3] = &stringBuffer[tempSortName_langIndices[3]];
    m_SortName_lang[4] = &stringBuffer[tempSortName_langIndices[4]];
    m_SortName_lang[5] = &stringBuffer[tempSortName_langIndices[5]];
    m_SortName_lang[6] = &stringBuffer[tempSortName_langIndices[6]];
    m_SortName_lang[7] = &stringBuffer[tempSortName_langIndices[7]];
  } else {
    m_SortName_lang[0] = "";
    m_SortName_lang[1] = "";
    m_SortName_lang[2] = "";
    m_SortName_lang[3] = "";
    m_SortName_lang[4] = "";
    m_SortName_lang[5] = "";
    m_SortName_lang[6] = "";
    m_SortName_lang[7] = "";
  }

  return true;
}
