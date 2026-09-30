#include <Base/Base.h>

#include "ItemSubClassRec.h"

#include <Console/ConsoleClient.h>

LPCSTR ItemSubClassRec::GetFilename() {
  return "DBFilesClient\\ItemSubClass.dbc";
}

ItemSubClassRec::ItemSubClassRec() {
}

ItemSubClassRec::~ItemSubClassRec() {
}

bool ItemSubClassRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempdisplayName_langIndices[NUM_LOCALES];
  UINT tempverboseName_langIndices[NUM_LOCALES];

  if (!SFileReadTyped(f, &m_classID) ||
      !SFileReadTyped(f, &m_subClassID) ||
      !SFileReadTyped(f, &m_prerequisiteProficiency) ||
      !SFileReadTyped(f, &m_postrequisiteProficiency) ||
      !SFileReadTyped(f, &m_flags) ||
      !SFileReadTyped(f, &m_displayFlags) ||
      !SFileReadTyped(f, &m_weaponParrySeq) ||
      !SFileReadTyped(f, &m_weaponReadySeq) ||
      !SFileReadTyped(f, &m_weaponAttackSeq) ||
      !SFileReadTyped(f, &m_WeaponSwingSize) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[0]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[1]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[2]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[3]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[4]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[5]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[6]) ||
      !SFileReadTyped(f, &tempdisplayName_langIndices[7]) ||
      !SFileReadTyped(f, &m_displayName_flag) ||
      !SFileReadTyped(f, &tempverboseName_langIndices[0]) ||
      !SFileReadTyped(f, &tempverboseName_langIndices[1]) ||
      !SFileReadTyped(f, &tempverboseName_langIndices[2]) ||
      !SFileReadTyped(f, &tempverboseName_langIndices[3]) ||
      !SFileReadTyped(f, &tempverboseName_langIndices[4]) ||
      !SFileReadTyped(f, &tempverboseName_langIndices[5]) ||
      !SFileReadTyped(f, &tempverboseName_langIndices[6]) ||
      !SFileReadTyped(f, &tempverboseName_langIndices[7]) ||
      !SFileReadTyped(f, &m_verboseName_flag)) {
    ConsoleWrite("Error reading ItemSubClassRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_displayName_lang[0] = stringBuffer + tempdisplayName_langIndices[0];
    m_displayName_lang[1] = stringBuffer + tempdisplayName_langIndices[1];
    m_displayName_lang[2] = stringBuffer + tempdisplayName_langIndices[2];
    m_displayName_lang[3] = stringBuffer + tempdisplayName_langIndices[3];
    m_displayName_lang[4] = stringBuffer + tempdisplayName_langIndices[4];
    m_displayName_lang[5] = stringBuffer + tempdisplayName_langIndices[5];
    m_displayName_lang[6] = stringBuffer + tempdisplayName_langIndices[6];
    m_displayName_lang[7] = stringBuffer + tempdisplayName_langIndices[7];
    m_verboseName_lang[0] = stringBuffer + tempverboseName_langIndices[0];
    m_verboseName_lang[1] = stringBuffer + tempverboseName_langIndices[1];
    m_verboseName_lang[2] = stringBuffer + tempverboseName_langIndices[2];
    m_verboseName_lang[3] = stringBuffer + tempverboseName_langIndices[3];
    m_verboseName_lang[4] = stringBuffer + tempverboseName_langIndices[4];
    m_verboseName_lang[5] = stringBuffer + tempverboseName_langIndices[5];
    m_verboseName_lang[6] = stringBuffer + tempverboseName_langIndices[6];
    m_verboseName_lang[7] = stringBuffer + tempverboseName_langIndices[7];
  } else {
    m_displayName_lang[0] = "";
    m_displayName_lang[1] = "";
    m_displayName_lang[2] = "";
    m_displayName_lang[3] = "";
    m_displayName_lang[4] = "";
    m_displayName_lang[5] = "";
    m_displayName_lang[6] = "";
    m_displayName_lang[7] = "";
    m_verboseName_lang[0] = "";
    m_verboseName_lang[1] = "";
    m_verboseName_lang[2] = "";
    m_verboseName_lang[3] = "";
    m_verboseName_lang[4] = "";
    m_verboseName_lang[5] = "";
    m_verboseName_lang[6] = "";
    m_verboseName_lang[7] = "";
  }

  return true;
}
