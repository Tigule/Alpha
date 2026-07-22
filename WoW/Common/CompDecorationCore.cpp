#include <Component/Component.h>
#include <Component/CharacterCustomization.h>
#include <Services/Texture.h>

#include "DB/DBClient/AutoCode/ChrRacesRec.h"

#include <storm.h>

static const char *const s_texComponentBasePath = "Item\\TextureComponents\\";
static char *const       s_sexSuffixNames[UNITSEX_LAST] = {"M", "F", "U"};

void __fastcall CompDecorateObjName(const char *string, char *buffer, unsigned int size, unsigned int race, unsigned int sex) {
  FATALASSERT(buffer);
  FATALASSERT(size);
  buffer[0] = 0;
  if (!string || !*string) {
    return;
  }

  FATALASSERT(race != 0);
  const ChrRacesRec *raceRec = g_chrRacesDB.GetRecord(race);
  FATALASSERT(raceRec);

  char inputFile[MAX_PATH];
  char extension[12] = "";
  SStrCopy(inputFile, string, sizeof(inputFile));
  char *dot = SStrChr(inputFile, '.');
  if (dot) {
    SStrCopy(extension, dot, sizeof(extension));
    *dot = 0;
  }
  SStrPrintf(buffer, size, "%s_%s%s%s", inputFile, raceRec->m_ClientPrefix, s_sexSuffixNames[sex], extension);
}

static void __fastcall
BuildTexComponentPath(const char *string, TEXCOMPONENT_SECTIONS section, char *buffer, unsigned int size, UNIT_SEX sex, int includeSex) {
  static const char *sectionNames[NUM_TEXCOMPONENT_SECTIONS] = {"ArmUpperTexture",  "ArmLowerTexture",   "HandTexture",       "HeadUpperTexture",
                                                                "HeadLowerTexture", "TorsoUpperTexture", "TorsoLowerTexture", "LegUpperTexture",
                                                                "LegLowerTexture",  "FootTexture"};
  char               stringBuffer[MAX_PATH];
  char               suffixBuffer[16];

  FATALASSERT(string);
  FATALASSERT(buffer);
  FATALASSERT(size);
  FATALASSERT(section < NUM_TEXCOMPONENT_SECTIONS);

  SStrCopy(stringBuffer, string, sizeof(stringBuffer));
  suffixBuffer[0] = 0;
  if (includeSex) {
    SStrPrintf(suffixBuffer, sizeof(suffixBuffer), "_%s", s_sexSuffixNames[sex]);
  }

  char *extension = SStrChr(stringBuffer, '.');
  if (extension) {
    *extension = 0;
  }

  SStrPrintf(buffer, size, "%s%s\\%s%s%s", s_texComponentBasePath, sectionNames[section], stringBuffer, suffixBuffer, ".BLP");
}

static int __fastcall ComponentUtilImageFileExists(const char *fileName) {
  if (SFile::FileExists(fileName)) {
    return 1;
  }

  char        alternate[MAX_PATH];
  TEXFILETYPE type = TextureDiscoverFileType(fileName);
  TexturePickAlternateFilename(fileName, type, alternate, sizeof(alternate));
  return SFile::FileExists(alternate);
}

void __fastcall
CompDecorateTexName(const char *string, TEXCOMPONENT_SECTIONS section, char *buffer, unsigned int size, unsigned int sex, int includeSex) {
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
  }
}
