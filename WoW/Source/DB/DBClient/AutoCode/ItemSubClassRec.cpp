#include "ItemSubClassRec.h"

#include <Console/ConsoleClient.h>

const char *__fastcall ItemSubClassRec::GetFilename() {
  return "DBFilesClient\\ItemSubClass.dbc";
}

ItemSubClassRec::ItemSubClassRec() {
}

ItemSubClassRec::~ItemSubClassRec() {
}

bool ItemSubClassRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempdisplayName_langIndices[NUM_LOCALES];
  unsigned int tempverboseName_langIndices[NUM_LOCALES];

  result = SFile::Read(f, &m_classID, sizeof(m_classID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_subClassID, sizeof(m_subClassID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_prerequisiteProficiency, sizeof(m_prerequisiteProficiency), 0, 0, 0) && result;
  result = SFile::Read(f, &m_postrequisiteProficiency, sizeof(m_postrequisiteProficiency), 0, 0, 0) && result;
  result = SFile::Read(f, &m_flags, sizeof(m_flags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_displayFlags, sizeof(m_displayFlags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_weaponParrySeq, sizeof(m_weaponParrySeq), 0, 0, 0) && result;
  result = SFile::Read(f, &m_weaponReadySeq, sizeof(m_weaponReadySeq), 0, 0, 0) && result;
  result = SFile::Read(f, &m_weaponAttackSeq, sizeof(m_weaponAttackSeq), 0, 0, 0) && result;
  result = SFile::Read(f, &m_WeaponSwingSize, sizeof(m_WeaponSwingSize), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[0], sizeof(tempdisplayName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[1], sizeof(tempdisplayName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[2], sizeof(tempdisplayName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[3], sizeof(tempdisplayName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[4], sizeof(tempdisplayName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[5], sizeof(tempdisplayName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[6], sizeof(tempdisplayName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempdisplayName_langIndices[7], sizeof(tempdisplayName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_displayName_flag, sizeof(m_displayName_flag), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverboseName_langIndices[0], sizeof(tempverboseName_langIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverboseName_langIndices[1], sizeof(tempverboseName_langIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverboseName_langIndices[2], sizeof(tempverboseName_langIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverboseName_langIndices[3], sizeof(tempverboseName_langIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverboseName_langIndices[4], sizeof(tempverboseName_langIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverboseName_langIndices[5], sizeof(tempverboseName_langIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverboseName_langIndices[6], sizeof(tempverboseName_langIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempverboseName_langIndices[7], sizeof(tempverboseName_langIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_verboseName_flag, sizeof(m_verboseName_flag), 0, 0, 0) && result;

  if (!result) {
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
