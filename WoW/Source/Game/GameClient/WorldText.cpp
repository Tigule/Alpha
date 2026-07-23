#include "WorldText.h"

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"

#include <Base/Coordinate.h>
#include <FrameScript/FrameScript.h>
#include <Gxu/IGxuFont.h>
#include <Services/TextBlock.h>

static TSExplicitList<WORLDTEXTSTRING, 0x180> s_textList;
static WORLDTEXTCREATEPARAMS                  s_worldTextParams[NUM_WORLDTEXTTYPES];
static HTEXTFONT                              s_worldTextFontHandles[NUM_WORLDTEXTTYPES];
static CVar                                  *s_fontHeightCVar;
static CVar                                  *s_fontOutlineCVar;
static CVar                                  *s_fontFadeInTime;
static CVar                                  *s_fontFadeOutTime;
static CVar                                  *s_fontFadeTotalTime;
static CVar                                  *s_fontFadeAscendDistance;
static CVar                                  *s_fontCharSpacing;
static CVar                                  *s_fontConeAngle;

static unsigned int const worldTextFlags[NUM_WORLDTEXTTYPES] = {0x48, 0x48, 0x10, 0x8, 0x48, 0, 0};

WORLDTEXTCREATEPARAMS::WORLDTEXTCREATEPARAMS() {
}

void WORLDTEXTCREATEPARAMS::Defaults() {
  ascendDistance = 24.0f;
  totalTime = 3000;
  fadeInTime = 1000;
  fadeOutTime = 2000;
  fontColor = NTempest::CImVector(255, 255, 255, 255);
  shadowOffset = NTempest::C2Vector(0.0f, 0.0f);
  shadowColor = NTempest::CImVector(255, 0, 0, 0);
  charSpacing = 0.0f;
  heightScale = 1.0f;
  zOffset = 0.0f;
  border = NTempest::C2Vector(0.0f, 0.0f);
  startFontHeight = 0.0125f;
  endFontHeight = 0.025f;
  enlargeTime = 0;
  shrinkTime = 0;
  flags = 0;
}

WORLDTEXTSTRING::WORLDTEXTSTRING() : worldTextType(NUM_WORLDTEXTTYPES), elapsedTime(0), totalTime(0), object(0), hidden(0), m_flags(0), string(0) {
  params.Defaults();
  savedStringText[0] = 0;
}

static int IntInterp(float progress, int start, int end) {
    // TODO: implement
    return 0;
}

WORLDTEXTSTRING::~WORLDTEXTSTRING() {
  if (string) {
    GxuFontDestroyString(string);
  }
}

void WORLDTEXTSTRING::Hide(int hide) {
  hidden = hide;
}

void WORLDTEXTSTRING::Render() {
  if (!hidden && string) {
    GxuFontRender(string);
  }
}

void WORLDTEXTSTRING::InitTextFrame(const char *text) {
  if (text && *text) {
    SStrPrintf(savedStringText, sizeof(savedStringText), "%s", text);
  }
}

static void __fastcall InitConsoleVariables() {
  s_fontHeightCVar = CVar::Register("DamageFontHeight", 0, 0, "0.035", 0, DEFAULT, false, 0);
  s_fontOutlineCVar = CVar::Register("DamageFontOutline", 0, 0, "1", 0, DEFAULT, false, 0);
  s_fontFadeInTime = CVar::Register("DamageFontFadeInTime", 0, 0, "200", 0, DEFAULT, false, 0);
  s_fontFadeOutTime = CVar::Register("DamageFontFadeOutTime", 0, 0, "900", 0, DEFAULT, false, 0);
  s_fontFadeTotalTime = CVar::Register("DamageFontTotalTime", 0, 0, "1200", 0, DEFAULT, false, 0);
  s_fontFadeAscendDistance = CVar::Register("DamageFontAscendDistance", 0, 0, "7", 0, DEFAULT, false, 0);
  s_fontCharSpacing = CVar::Register("DamageFontCharSpacing", 0, 0, "-0.001", 0, DEFAULT, false, 0);
  s_fontConeAngle = CVar::Register("DamageFontConeAngle", 0, 0, "0", 0, DEFAULT, false, 0);
}

void __fastcall WorldTextInitialize() {
  unsigned int i;
  for (i = 0; i < NUM_WORLDTEXTTYPES; ++i) {
    s_worldTextParams[i].Defaults();
  }

  InitConsoleVariables();

  float        fontHeight = NDCToDDCHeight(s_fontHeightCVar->GetFloat());
  float        critFontHeight = fontHeight * 1.5f;
  unsigned int fontFlags[NUM_WORLDTEXTTYPES] = {4, 4, 4, 0, 4, 0, 0};
  float        fontHeights[NUM_WORLDTEXTTYPES] = {fontHeight, fontHeight, critFontHeight, fontHeight, fontHeight, fontHeight, fontHeight};

  const char *fontName = FrameScript_GetText("DAMAGE_TEXT_FONT", -1, GENDER_NOT_APPLICABLE);
  if (fontName && *fontName) {
    for (i = 0; i < NUM_WORLDTEXTTYPES; ++i) {
      FATALASSERT(!s_worldTextFontHandles[i]);
      s_worldTextFontHandles[i] = TextBlockGenerateFont(fontName, fontFlags[i], DDCToNDCHeight(fontHeights[i]));
    }
  }

  static NTempest::CImVector colorArray[NUM_WORLDTEXTTYPES] = {NTempest::CImVector(0xFFFFFFFF), NTempest::CImVector(0xFFFFFFFF),
                                                               NTempest::CImVector(0xFFFFFFFF), NTempest::CImVector(0xFF00FF00),
                                                               NTempest::CImVector(0xFFFFFFFF), NTempest::CImVector(0x8094008B),
                                                               NTempest::CImVector(0xFFFF8040)};

  float ascendDistance = s_fontFadeAscendDistance->GetFloat() * 0.027777778f;
  struct {
    unsigned int fadeInTime;
    unsigned int fadeOutTime;
    unsigned int totalTime;
    float        ascendDist;
    float        smallFontHeight;
    float        largeFontontHeight;
    float        heightScale;
    float        heightOffset;
    unsigned int enlargeTime;
    unsigned int shrinkTime;
  } worldTextInfo[NUM_WORLDTEXTTYPES] = {
      {                                                    0,                                                    800,800,        ascendDistance, fontHeight,     fontHeight, 1.0f, 0.27777778f, 0, 0                                                                                                                     },
      {                                                    0,                                                    800,                           800,        ascendDistance, fontHeight,     fontHeight, 1.0f, 0.27777778f, 0, 0},
      {                                                    0,                                                    800,                           800,                  0.0f,       0.0f, critFontHeight, 1.0f, 0.27777778f, 0, 0},
      {static_cast<unsigned int>(s_fontFadeInTime->GetInt()), static_cast<unsigned int>(s_fontFadeOutTime->GetInt()),
       static_cast<unsigned int>(s_fontFadeTotalTime->GetInt()),        ascendDistance, fontHeight,     fontHeight, 1.0f, 0.27777778f, 0, 0                                                                                    },
      {                                                    0,                                                    800,                           800,        ascendDistance, fontHeight,     fontHeight, 1.0f, 0.27777778f, 0, 0},
      {                                                  500,                                                    400,                          6000, ascendDistance * 2.0f, fontHeight,     fontHeight, 1.0f,        0.0f, 0, 0},
      {                                                    0,                          static_cast<unsigned int>(-1), static_cast<unsigned int>(-1),                  0.0f, fontHeight,     fontHeight, 1.0f, 0.27777778f, 0, 0}
  };

  for (i = 0; i < NUM_WORLDTEXTTYPES; ++i) {
    WORLDTEXTCREATEPARAMS &params = s_worldTextParams[i];
    params.ascendDistance = worldTextInfo[i].ascendDist;
    params.totalTime = worldTextInfo[i].totalTime;
    params.fadeInTime = worldTextInfo[i].fadeInTime;
    params.fadeOutTime = worldTextInfo[i].fadeOutTime;
    params.fontColor = colorArray[i];
    params.shadowOffset = NTempest::C2Vector(0.001f, -0.001f);
    params.shadowColor = NTempest::CImVector(126, 255, 255, 255);
    params.charSpacing = s_fontCharSpacing->GetFloat();
    params.heightScale = worldTextInfo[i].heightScale;
    params.zOffset = worldTextInfo[i].heightOffset;
    params.border = NTempest::C2Vector(0.0f, 0.03f);
    params.startFontHeight = worldTextInfo[i].smallFontHeight;
    params.endFontHeight = worldTextInfo[i].largeFontontHeight;
    params.enlargeTime = worldTextInfo[i].enlargeTime;
    params.shrinkTime = worldTextInfo[i].shrinkTime;
    params.flags = worldTextFlags[i];
  }
}

void __fastcall WorldTextShutdown() {
  for (unsigned int i = 0; i < NUM_WORLDTEXTTYPES; ++i) {
    if (s_worldTextFontHandles[i]) {
      HandleClose(s_worldTextFontHandles[i]);
    }
    s_worldTextFontHandles[i] = 0;
  }
}

void __fastcall WorldTextClearStrings() {
  WORLDTEXTSTRING *worldText = s_textList.Head();

  while (worldText) {
    if (worldText->string) {
      GxuFontDestroyString(worldText->string);
    }

    worldText->string = 0;
    worldText = s_textList.Next(worldText);
  }
}

void __fastcall WorldTextGetColor(WORLDTEXTTYPE type, NTempest::CImVector* color) {
    // TODO: implement
}

HWORLDTEXT__ *__fastcall WorldTextCreate(WORLDTEXTTYPE type, const char *text, unsigned __int64 object, const NTempest::CImVector *colorOverride) {
  FATALASSERT(type < NUM_WORLDTEXTTYPES);
  WORLDTEXTCREATEPARAMS *params = &s_worldTextParams[type];
  FATALASSERT(params);
  FATALASSERT(params->fadeInTime <= params->totalTime);
  FATALASSERT(params->fadeOutTime <= params->totalTime);

  WORLDTEXTSTRING *worldTextPtr = NEW(WORLDTEXTSTRING);
  FATALASSERT(worldTextPtr);
  s_textList.LinkNode(worldTextPtr, LIST_HEAD, 0);
  worldTextPtr->worldTextType = type;
  worldTextPtr->object = object;
  worldTextPtr->totalTime = params->totalTime;
  worldTextPtr->params = *params;
  if (colorOverride) {
    worldTextPtr->params.fontColor = *colorOverride;
  }
  FATALASSERT(worldTextPtr->totalTime);
  FATALASSERT(params->enlargeTime <= params->shrinkTime);
  FATALASSERT(params->enlargeTime <= worldTextPtr->totalTime);
  FATALASSERT(params->shrinkTime <= worldTextPtr->totalTime);
  worldTextPtr->InitTextFrame(text);
  return reinterpret_cast<HWORLDTEXT__ *>(HandleCreate(worldTextPtr, "HWORLDTEXT"));
}

void __fastcall WorldTextUpdate(float elapsed, const NTempest::C44Matrix& matrix) {
    // TODO: implement
}

void __fastcall WorldTextUpdate(HWORLDTEXT__* text, float elapsed, const NTempest::C44Matrix& matrix, const NTempest::C3Vector* position) {
    // TODO: implement
}

int __fastcall WorldTextIsTextDone(HWORLDTEXT__* handle) {
    // TODO: implement
    return 0;
}

void __fastcall WorldTextShow(HWORLDTEXT__ *text, int show) {
  if (text) {
    reinterpret_cast<WORLDTEXTSTRING *>(text)->Hide(!show);
  }
}

void __fastcall WorldTextRender(HWORLDTEXT__ *text) {
  if (text) {
    reinterpret_cast<WORLDTEXTSTRING *>(text)->Render();
  }
}
