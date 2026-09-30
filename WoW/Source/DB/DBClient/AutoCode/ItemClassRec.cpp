#include "ItemClassRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR ItemClassRec::GetFilename() {
  return "DBFilesClient\\ItemClass.dbc";
}

ItemClassRec::ItemClassRec() {
}

ItemClassRec::~ItemClassRec() {
}

bool ItemClassRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempclassName_langIndices[NUM_LOCALES];

  if (!SFileReadTyped(f, &m_classID) ||
      !SFileReadTyped(f, &m_subclassMapID) ||
      !SFileReadTyped(f, &m_flags) ||
      !SFileReadTyped(f, &tempclassName_langIndices[0]) ||
      !SFileReadTyped(f, &tempclassName_langIndices[1]) ||
      !SFileReadTyped(f, &tempclassName_langIndices[2]) ||
      !SFileReadTyped(f, &tempclassName_langIndices[3]) ||
      !SFileReadTyped(f, &tempclassName_langIndices[4]) ||
      !SFileReadTyped(f, &tempclassName_langIndices[5]) ||
      !SFileReadTyped(f, &tempclassName_langIndices[6]) ||
      !SFileReadTyped(f, &tempclassName_langIndices[7]) ||
      !SFileReadTyped(f, &m_className_flag)) {
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
