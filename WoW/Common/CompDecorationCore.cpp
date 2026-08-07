#include <Base/Base.h>

#include <Component/Component.h>
#include <Component/CharacterCustomization.h>
#include <Services/SysMessage.h>
#include <Services/Texture.h>

#include "DB/DBClient/AutoCode/ChrRacesRec.h"

#include <storm.h>

static LPCSTR const s_texComponentBasePath = "Item\\TextureComponents\\";
static char *const  s_sexSuffixNames[UNITSEX_LAST] = {"M", "F", "U"};

static void ConstructSuffixString(UINT race, UINT sex, int includeRace, int includeSex, char *buffer, UINT size) {
  FATALASSERT(buffer);
  FATALASSERT(size);

  buffer[0] = 0;
  if (includeRace) {
    FATALASSERT(!includeRace || includeSex);
    const ChrRacesRec *rec = g_chrRacesDB.GetRecord(race);
    FATALASSERT(rec);
    SStrPrintf(buffer, size, "_%s%s", rec->m_ClientPrefix, s_sexSuffixNames[sex]);
  } else if (includeSex) {
    SStrPrintf(buffer, size, "_%s", s_sexSuffixNames[sex]);
  }
}

static void BuildObjComponentPath(LPCSTR fileName, UINT race, UINT sex, char *buffer, UINT size) {
  char inputFile[MAX_PATH];
  char filenameExtension[12] = "";
  char suffixBuffer[12];
  char finalBuffer[MAX_PATH];

  FATALASSERT(fileName);
  FATALASSERT(buffer);
  FATALASSERT(size);
  FATALASSERT(race != 0);
  FATALASSERT(race <= (UINT)g_chrRacesDB.GetMaxID());

  SStrCopy(inputFile, fileName, sizeof(inputFile));
  char *extension = SStrChrR(inputFile, '.');
  if (extension) {
    SStrCopy(filenameExtension, extension, sizeof(filenameExtension));
    *extension = 0;
  }

  ConstructSuffixString(race, sex, 1, 1, suffixBuffer, sizeof(suffixBuffer));
  SStrPrintf(finalBuffer, sizeof(finalBuffer), "%s%s%s", inputFile, suffixBuffer, filenameExtension);
  SStrCopy(buffer, finalBuffer, size);
}

void CompDecorateObjName(LPCSTR string, char *buffer, UINT size, UINT race, UINT sex) {
  FATALASSERT(buffer);
  FATALASSERT(size);
  buffer[0] = 0;
  if (!string || !*string) {
    return;
  }

  char finalName[MAX_PATH];
  BuildObjComponentPath(string, race, sex, finalName, size);
  if (!finalName[0]) {
    BuildObjComponentPath(string, race, UNITSEX_NONE, finalName, size);
  }
  if (finalName[0]) {
    SStrCopy(buffer, finalName, size);
  } else {
    SysMsgPrintf(SYSMSG_ERROR, "Error, object component file \"%s\" not found!", finalName);
  }
}

static void BuildTexComponentPath(LPCSTR string, TEXCOMPONENT_SECTIONS section, char *buffer, UINT size, UNIT_SEX sex, int includeSex) {
  static LPCSTR sectionNames[NUM_TEXCOMPONENT_SECTIONS] = {"ArmUpperTexture",  "ArmLowerTexture",   "HandTexture",       "HeadUpperTexture",
                                                           "HeadLowerTexture", "TorsoUpperTexture", "TorsoLowerTexture", "LegUpperTexture",
                                                           "LegLowerTexture",  "FootTexture"};
  char          stringBuffer[MAX_PATH];
  char          suffixBuffer[16];
  char          extension[16] = "";

  FATALASSERT(string);
  FATALASSERT(buffer);
  FATALASSERT(size);
  FATALASSERT(section < NUM_TEXCOMPONENT_SECTIONS);

  SStrCopy(stringBuffer, string, sizeof(stringBuffer));
  ConstructSuffixString(0, sex, 0, includeSex, suffixBuffer, sizeof(suffixBuffer));

  char *extensionPtr = SStrChrR(stringBuffer, '.');
  if (extensionPtr) {
    SStrCopy(extension, extensionPtr, sizeof(extension));
    *extensionPtr = 0;
  }

  SStrPrintf(buffer, size, "%s%s\\%s%s%s", s_texComponentBasePath, sectionNames[section], stringBuffer, suffixBuffer, ".BLP");
}

static int ComponentUtilImageFileExists(LPCSTR fileName) {
  if (SFile::FileExists(fileName)) {
    return 1;
  }

  char        alternate[MAX_PATH];
  TEXFILETYPE type = TextureDiscoverFileType(fileName);
  TexturePickAlternateFilename(fileName, type, alternate, sizeof(alternate));
  return SFile::FileExists(alternate);
}

void CompDecorateTexName(LPCSTR string, TEXCOMPONENT_SECTIONS section, char *buffer, UINT size, UINT sex, int includeSex) {
  char finalName[MAX_PATH];

  ASSERT(section < NUM_TEXCOMPONENT_SECTIONS);
  ASSERT(buffer);
  ASSERT(size);
  buffer[0] = 0;

  if (!string || !*string || section >= NUM_TEXCOMPONENT_SECTIONS) {
    return;
  }

  BuildTexComponentPath(string, section, finalName, sizeof(finalName), UNITSEX_NONE, includeSex);
  if (finalName[0] && ComponentUtilImageFileExists(finalName)) {
    SStrCopy(buffer, finalName, size);
    return;
  }

  BuildTexComponentPath(string, section, finalName, sizeof(finalName), static_cast<UNIT_SEX>(sex), includeSex);
  if (finalName[0] && ComponentUtilImageFileExists(finalName)) {
    SStrCopy(buffer, finalName, size);
  } else {
    SysMsgPrintf(SYSMSG_ERROR, "Error, texture component file \"%s\" not found!", finalName);
  }
}

BOOL CompDecorateUndecorateObjName(LPCSTR string, char *buffer, UINT size) {
  enum STATES {
    FINDING_PERIOD = 0,
    FINDING_DIRECTORYSEPARATOR = 1,
    FINDING_PURGEDIRECTORYSEPARATOR = 2
  };

  FATALASSERT(buffer);
  FATALASSERT(size);

  buffer[0] = 0;
  if (!string || !*string) {
    return 0;
  }

  SStrCopy(buffer, string, size);
  char   extensionBuffer[12] = "";
  char   fileNameBuffer[MAX_PATH] = "";
  STATES state = FINDING_PERIOD;
  char  *cursor = buffer + SStrLen(buffer) - 1;
  for (; cursor >= buffer; --cursor) {
    if (state == FINDING_PERIOD) {
      if (*cursor == '.') {
        SStrCopy(extensionBuffer, cursor, sizeof(extensionBuffer));
        *cursor = 0;
        state = FINDING_DIRECTORYSEPARATOR;
      }
    } else if (state == FINDING_DIRECTORYSEPARATOR) {
      if (*cursor == '\\') {
        SStrCopy(fileNameBuffer, cursor + 1, sizeof(fileNameBuffer));
        char *suffix = SStrChrR(fileNameBuffer, '_');
        if (suffix) {
          *suffix = 0;
        }
        state = FINDING_PURGEDIRECTORYSEPARATOR;
      }
    } else if (*cursor == '\\') {
      cursor[1] = 0;
      SStrPack(buffer, fileNameBuffer, size);
      SStrPack(buffer, extensionBuffer, size);
      return 1;
    }
  }

  SStrCopy(buffer, string, size);
  return 0;
}

BOOL CompDecorateUndecorateTexName(LPCSTR string, char *buffer, UINT size) {
  enum STATES {
    FINDING_PERIOD = 0,
    FINDING_UNDERSCORE = 1
  };

  FATALASSERT(buffer);
  FATALASSERT(size);

  buffer[0] = 0;
  if (!string || !*string) {
    return 0;
  }

  SStrCopy(buffer, string, size);
  char   extension[12] = "";
  STATES state = FINDING_PERIOD;
  char  *cursor = buffer + SStrLen(buffer) - 1;
  for (; cursor >= buffer; --cursor) {
    if (state == FINDING_PERIOD && *cursor == '.') {
      SStrCopy(extension, cursor, sizeof(extension));
      state = FINDING_UNDERSCORE;
    } else if (state == FINDING_UNDERSCORE && *cursor == '_') {
      *cursor = 0;
      FATALASSERT(extension[0]);
      SStrPack(buffer, extension, size);
      return 1;
    }
  }

  SStrCopy(buffer, string, size);
  return 0;
}
