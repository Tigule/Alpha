#include "QuestSortRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *QuestSortRec::GetFilename() {
  return "DBFilesClient\\QuestSort.dbc";
}

QuestSortRec::QuestSortRec() {
}

QuestSortRec::~QuestSortRec() {
}

bool QuestSortRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempSortName_langIndices[NUM_LOCALES];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &tempSortName_langIndices[0], sizeof(tempSortName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempSortName_langIndices[1], sizeof(tempSortName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempSortName_langIndices[2], sizeof(tempSortName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempSortName_langIndices[3], sizeof(tempSortName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempSortName_langIndices[4], sizeof(tempSortName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempSortName_langIndices[5], sizeof(tempSortName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempSortName_langIndices[6], sizeof(tempSortName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempSortName_langIndices[7], sizeof(tempSortName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SortName_flag, sizeof(m_SortName_flag), 0, 0, 0) && result;

  if (!result) {
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
