#include "ItemClassRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall ItemClassRec::GetFilename() {
  return "DBFilesClient\\ItemClass.dbc";
}

ItemClassRec::ItemClassRec() {
}

ItemClassRec::~ItemClassRec() {
}

bool ItemClassRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempclassName_langIndices[NUM_LOCALES];

  result = SFile::Read(f, &m_classID, sizeof(m_classID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_subclassMapID, sizeof(m_subclassMapID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_flags, sizeof(m_flags), 0, 0, 0) && result;
  result = SFile::Read(f, &tempclassName_langIndices[0], sizeof(tempclassName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempclassName_langIndices[1], sizeof(tempclassName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempclassName_langIndices[2], sizeof(tempclassName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempclassName_langIndices[3], sizeof(tempclassName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempclassName_langIndices[4], sizeof(tempclassName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempclassName_langIndices[5], sizeof(tempclassName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempclassName_langIndices[6], sizeof(tempclassName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempclassName_langIndices[7], sizeof(tempclassName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_className_flag, sizeof(m_className_flag), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading ItemClassRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_className_lang[0] = &stringBuffer[tempclassName_langIndices[0]];
    m_className_lang[1] = &stringBuffer[tempclassName_langIndices[1]];
    m_className_lang[2] = &stringBuffer[tempclassName_langIndices[2]];
    m_className_lang[3] = &stringBuffer[tempclassName_langIndices[3]];
    m_className_lang[4] = &stringBuffer[tempclassName_langIndices[4]];
    m_className_lang[5] = &stringBuffer[tempclassName_langIndices[5]];
    m_className_lang[6] = &stringBuffer[tempclassName_langIndices[6]];
    m_className_lang[7] = &stringBuffer[tempclassName_langIndices[7]];
  } else {
    m_className_lang[0] = "";
    m_className_lang[1] = "";
    m_className_lang[2] = "";
    m_className_lang[3] = "";
    m_className_lang[4] = "";
    m_className_lang[5] = "";
    m_className_lang[6] = "";
    m_className_lang[7] = "";
  }

  return true;
}
