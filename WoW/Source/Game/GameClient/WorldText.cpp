#include "WorldText.h"

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "Object/ObjectClient/Object_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/WorldFrame.h"

#include <Base/Coordinate.h>
#include <FrameScript/FrameScript.h>
#include <Gxu/IGxuFont.h>
#include <Services/TextBlock.h>
#include <Tempest/c4vector.h>

#include <math.h>

static const float PI = 3.14159265358979323846f;

static LISTDECLEX(WORLDTEXTSTRING, link, s_textList);
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

struct WORLDTEXTRANGE {
  float startProgress;
  float endProgress;
  float startScale;
  float endScale;
};

struct WORLDTEXTJUMP {
  float startProgress;
  float endProgress;
  float heightScale;
};

struct WORLDTEXTFONTCOLOR {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char pad;
  float         unused[3];
  float         startProgress;
  float         endProgress;
};

static WORLDTEXTJUMP s_jumps[3] = {
    {-0.33f, 0.33f, 1.5f},
    { 0.33f, 0.5f,  0.2f},
    { 0.5f,  0.66f, 0.1f}
};
static WORLDTEXTRANGE s_critHeights[3] = {
    {0.0f, 0.1f, 0.1f, 0.0f},
    {0.1f, 0.2f, 0.0f, 1.0f},
    {0.2f, 1.0f, 1.0f, 1.0f}
};
static WORLDTEXTRANGE s_critFades[3] = {
    {0.0f, 0.1f, 0.0f, 1.0f},
    {0.1f, 0.5f, 1.0f, 1.0f},
    {0.5f, 1.0f, 1.0f, 0.0f}
};
static WORLDTEXTFONTCOLOR s_fontColors[1] = {
    {255, 0, 0, 0, {0.0f, 0.0f, 0.0f}, 0.0f, 0.33f}
};

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
  return static_cast<int>(static_cast<double>(end - start) * progress + static_cast<double>(start));
}

static NTempest::CImVector ColorInterp(float range, NTempest::CImVector startColor, NTempest::CImVector endColor) {
  if (range > 1.0f) {
    range = 1.0f;
  }
  return NTempest::CImVector(
      255,
      static_cast<unsigned char>(IntInterp(range, startColor.r, endColor.r)),
      static_cast<unsigned char>(IntInterp(range, startColor.g, endColor.g)),
      static_cast<unsigned char>(IntInterp(range, startColor.b, endColor.b))
  );
}

WORLDTEXTSTRING::~WORLDTEXTSTRING() {
  if (string) {
    GxuFontDestroyString(string);
  }
}

void WORLDTEXTSTRING::Hide(int hide) {
  hidden = hide;
}

void WORLDTEXTSTRING::Render() const {
  if (!hidden && string) {
    GxuFontRender(string);
  }
}

void WORLDTEXTSTRING::InitTextFrame(const char *text) {
  if (text && *text) {
    SStrPrintf(savedStringText, sizeof(savedStringText), "%s", text);
  }
}

static void InitConsoleVariables() {
  s_fontHeightCVar = CVar::Register("DamageFontHeight", 0, 0, "0.035", 0, DEFAULT, false, 0);
  s_fontOutlineCVar = CVar::Register("DamageFontOutline", 0, 0, "1", 0, DEFAULT, false, 0);
  s_fontFadeInTime = CVar::Register("DamageFontFadeInTime", 0, 0, "200", 0, DEFAULT, false, 0);
  s_fontFadeOutTime = CVar::Register("DamageFontFadeOutTime", 0, 0, "900", 0, DEFAULT, false, 0);
  s_fontFadeTotalTime = CVar::Register("DamageFontTotalTime", 0, 0, "1200", 0, DEFAULT, false, 0);
  s_fontFadeAscendDistance = CVar::Register("DamageFontAscendDistance", 0, 0, "7", 0, DEFAULT, false, 0);
  s_fontCharSpacing = CVar::Register("DamageFontCharSpacing", 0, 0, "-0.001", 0, DEFAULT, false, 0);
  s_fontConeAngle = CVar::Register("DamageFontConeAngle", 0, 0, "0", 0, DEFAULT, false, 0);
}

void WorldTextInitialize() {
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

void WorldTextShutdown() {
  for (unsigned int i = 0; i < NUM_WORLDTEXTTYPES; ++i) {
    if (s_worldTextFontHandles[i]) {
      HandleClose(s_worldTextFontHandles[i]);
    }
    s_worldTextFontHandles[i] = 0;
  }
}

void WorldTextClearStrings() {
  ITERATELIST(WORLDTEXTSTRING, s_textList, worldText) {
    if (worldText->string) {
      GxuFontDestroyString(worldText->string);
    }

    worldText->string = 0;
  }
}

void WorldTextGetColor(WORLDTEXTTYPE type, NTempest::CImVector* color) {
  FATALASSERT(type < NUM_WORLDTEXTTYPES);
  FATALASSERT(color);
  *color = s_worldTextParams[type].fontColor;
}

HWORLDTEXT__ *WorldTextCreate(WORLDTEXTTYPE type, const char *text, unsigned __int64 object, const NTempest::CImVector *colorOverride) {
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

void WorldTextUpdate(float elapsed, const NTempest::C44Matrix& matrix) {
  ITERATELIST(WORLDTEXTSTRING, s_textList, text) {
    if (text->object) {
      text->Update(elapsed, matrix, 0);
    }
  }
}

void WORLDTEXTSTRING::Reset() {
  elapsedTime = 0;
}

void WORLDTEXTSTRING::UpdatePosition(
    const NTempest::C4Vector &worldPosition,
    unsigned int elapsed,
    NTempest::C4Vector &textPos
) {
  textPos = worldPosition;
  if (!(params.flags & 1)) {
    float progress = static_cast<float>(elapsed > 1 ? elapsed : 1) / static_cast<float>(totalTime);
    float distance = params.ascendDistance;
    if (params.flags & 8) {
      unsigned int i;
      for (i = 0; i < 3; ++i) {
        if (progress >= s_jumps[i].startProgress && progress <= s_jumps[i].endProgress) {
          float range = s_jumps[i].endProgress - s_jumps[i].startProgress;
          float rangeProgress = range == 0.0f ? 1.0f : (progress - s_jumps[i].startProgress) / range;
          float curve = static_cast<float>(sin(rangeProgress * 3.1415927f));
          distance *= curve * s_jumps[i].heightScale * curve * curve;
          break;
        }
      }
      if (i == 3) {
        distance = 0.0f;
      }
    } else {
      distance *= progress;
    }
    textPos.z += distance;
  }
}

void WORLDTEXTSTRING::CalculateNewPosition(
    const NTempest::C4Vector &worldPosition,
    unsigned int elapsed,
    NTempest::C4Vector &textPos,
    const NTempest::C44Matrix &matrix,
    int worldPositionSpecified
) {
  UpdatePosition(worldPosition, elapsed, textPos);
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  ASSERT(worldFrame);
  textPos = worldFrame->GetScreenCoordinates(
      NTempest::C3Vector(textPos.x, textPos.y, textPos.z), matrix, 1, worldPositionSpecified
  );
  textPos.w = 1.0f;
}

void WORLDTEXTSTRING::Update(float elapsed, const NTempest::C44Matrix &matrix, const NTempest::C3Vector *basePosition) {
  ASSERT(object || basePosition);
  if (hidden) {
    return;
  }

  CalculateTextHeight(elapsedTime);
  if (!string) {
    return;
  }

  elapsedTime += static_cast<unsigned int>(elapsed * 1000.0f);
  if (elapsedTime > totalTime && !(params.flags & 1)) {
    return;
  }

  CalculateNewColor(elapsedTime);
  NTempest::C4Vector worldPosition;
  CGObject_C *obj = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
  int worldPositionSpecified = 0;
  if (obj) {
    NTempest::C3Vector position;
    obj->GetPosition(position);
    worldPosition = NTempest::C4Vector(position.x, position.y, position.z + obj->GetScale(), 1.0f);
    worldPositionSpecified = 1;
  } else {
    worldPosition = NTempest::C4Vector(basePosition->x, basePosition->y, basePosition->z + params.zOffset, 1.0f);
  }

  NTempest::C4Vector textPosition;
  CalculateNewPosition(worldPosition, elapsedTime, textPosition, matrix, worldPositionSpecified);
  if (!(params.flags & 1)
      || (textPosition.x >= 0.0f && textPosition.y >= 0.0f && textPosition.x < 1.0f && textPosition.y < 1.0f)) {
    float screenX;
    float screenY;
    NDCToDDC(textPosition.x, textPosition.y, &screenX, &screenY);
    float halfWidth = textWidth * 0.5f;
    float halfHeight = textHeight * 0.5f;
    float borderX = halfWidth + params.border.x;
    float borderY = halfHeight + params.border.y;
    screenX = __max(borderX, __min(screenX, 0.8f - borderX));
    screenY = __max(borderY, __min(screenY, 0.6f - borderY));
    NTempest::C3Vector position(DDCToNDCWidth(screenX) - halfWidth, DDCToNDCHeight(screenY), 0.0f);
    GxuFontSetStringPosition(string, position);
  }
}

void WORLDTEXTSTRING::CalculateTextHeight(unsigned int elapsed) {
  unsigned int time = elapsed < params.totalTime ? elapsed : params.totalTime;
  float heightScale;

  if (params.flags & 0x10) {
    float progress = static_cast<float>(time) / static_cast<float>(params.totalTime);
    heightScale = params.endFontHeight;
    for (unsigned int i = 0; i < 3; ++i) {
      ASSERT(s_critHeights[i].startProgress <= s_critHeights[i].endProgress);
      if (progress >= s_critHeights[i].startProgress && progress < s_critHeights[i].endProgress) {
        float range = (progress - s_critHeights[i].startProgress)
                    / (s_critHeights[i].endProgress - s_critHeights[i].startProgress);
        heightScale = (range * (s_critHeights[i].endScale - s_critHeights[i].startScale)
                + s_critHeights[i].startScale) * params.endFontHeight;
        break;
      }
    }
  } else if (!(params.flags & 1) && time < params.enlargeTime) {
    heightScale = static_cast<float>(time) / static_cast<float>(params.enlargeTime)
           * (params.endFontHeight - params.startFontHeight) + params.startFontHeight;
  } else if (!(params.flags & 1) && time >= params.shrinkTime && time < totalTime) {
    heightScale = static_cast<float>(totalTime - time) / static_cast<float>(totalTime - params.shrinkTime)
           * (params.endFontHeight - params.startFontHeight) + params.startFontHeight;
  } else {
    heightScale = params.endFontHeight;
  }

  UpdateStringHeight(heightScale > 0.001f ? heightScale : 0.001f);
}

void WORLDTEXTSTRING::RecreateString() {
  ASSERT(worldTextType < NUM_WORLDTEXTTYPES);
  if (string) {
    GxuFontDestroyString(string);
  }
  string = 0;

  if ((m_flags & 0x80) && savedStringText[0] && s_worldTextFontHandles[worldTextType]) {
    CGxFont *font = TextBlockGetFontPtr(s_worldTextFontHandles[worldTextType]);
    float stringHeight = DDCToNDCHeight(savedStringHeight);
    GxuFontGetTextExtent(font, savedStringText, SStrLen(savedStringText), stringHeight, &textWidth, 0.0f, 0);
    textHeight = GxuFontGetWrappedTextHeight(font, savedStringText, stringHeight, textWidth, 0.0f, 0);
    GxuFontCreateString(
        font, savedStringText, stringHeight, NTempest::C3Vector(0.0f), textWidth, textHeight, 0.0f, string,
        GxVJ_Bottom, GxHJ_Left, 0, params.fontColor, 0.0f
    );
    if (string && (params.shadowOffset.x != 0.0f || params.shadowOffset.y != 0.0f)) {
      GxuFontAddShadow(string, params.shadowColor, params.shadowOffset);
    }
  }
}

void WORLDTEXTSTRING::UpdateStringHeight(float height) {
  if (!(m_flags & 0x80) || height != savedStringHeight) {
    m_flags |= 0x80;
    savedStringHeight = height;
    RecreateString();
  }
}

void WORLDTEXTSTRING::CalculateNewColor(unsigned int elapsed) {
  ASSERT(string);
  unsigned int time = elapsedTime < params.totalTime ? elapsedTime : params.totalTime;
  elapsedTime = time;
  float progress = static_cast<float>(time) / static_cast<float>(params.totalTime);
  unsigned char alpha = 255;
  unsigned char shadowAlpha = params.shadowColor.a;

  if (params.flags & 0x10) {
    float scale = 1.0f;
    for (unsigned int i = 0; i < 3; ++i) {
      ASSERT(s_critFades[i].startProgress <= s_critFades[i].endProgress);
      if (progress >= s_critFades[i].startProgress && progress < s_critFades[i].endProgress) {
        float range = (progress - s_critFades[i].startProgress)
                    / (s_critFades[i].endProgress - s_critFades[i].startProgress);
        scale = range * (s_critFades[i].endScale - s_critFades[i].startScale) + s_critFades[i].startScale;
        break;
      }
    }
    alpha = static_cast<unsigned char>(__min(255, static_cast<int>(scale * 255.0f)));
    shadowAlpha = static_cast<unsigned char>(__min(static_cast<int>(params.shadowColor.a),
                                                  static_cast<int>(params.shadowColor.a * scale)));
  } else if (!(params.flags & 1)) {
    if (elapsed < params.fadeInTime) {
      alpha = static_cast<unsigned char>(__min(255, static_cast<int>(progress * 255.0f)));
      shadowAlpha = static_cast<unsigned char>(
          __min(static_cast<int>(params.shadowColor.a), static_cast<int>(params.shadowColor.a * progress))
      );
    } else if (elapsed >= params.fadeOutTime) {
      float fade = totalTime == params.fadeOutTime
                 ? 0.0f
                 : static_cast<float>(elapsed - params.fadeOutTime) / static_cast<float>(totalTime - params.fadeOutTime);
      fade = __max(0.0f, __min(fade, 1.0f));
      alpha = static_cast<unsigned char>(255.0f - fade * 255.0f);
      shadowAlpha = static_cast<unsigned char>(255.0f - __min(static_cast<float>(params.shadowColor.a),
                                                              fade * params.shadowColor.a));
    }
  }

  NTempest::CImVector color = params.fontColor;
  if ((params.flags & 0x40) && progress >= s_fontColors[0].startProgress && progress < s_fontColors[0].endProgress) {
    float range = (progress - s_fontColors[0].startProgress)
                / (s_fontColors[0].endProgress - s_fontColors[0].startProgress);
    color = ColorInterp(range, color, NTempest::CImVector(255, s_fontColors[0].r, s_fontColors[0].g, s_fontColors[0].b));
  }
  color.a = alpha;
  GxuFontSetStringColor(string, color);

  if (params.shadowOffset.x != 0.0f || params.shadowOffset.y != 0.0f) {
    NTempest::CImVector shadow = params.shadowColor;
    shadow.a = shadowAlpha;
    GxuFontAddShadow(string, shadow, params.shadowOffset);
  }
}

void WorldTextUpdate(HWORLDTEXT__* text, float elapsed, const NTempest::C44Matrix& matrix, const NTempest::C3Vector* position) {
  if (text) {
    reinterpret_cast<WORLDTEXTSTRING *>(text)->Update(elapsed, matrix, position);
  }
}

int WorldTextIsTextDone(HWORLDTEXT__* handle) {
  WORLDTEXTSTRING *text = reinterpret_cast<WORLDTEXTSTRING *>(handle);
  if (!text) {
    return 1;
  }
  return text->elapsedTime > text->totalTime && !(text->m_flags & 1);
}

void WorldTextShow(HWORLDTEXT__ *text, int show) {
  if (text) {
    reinterpret_cast<WORLDTEXTSTRING *>(text)->Hide(!show);
  }
}

void WorldTextRender(HWORLDTEXT__ *text) {
  if (text) {
    reinterpret_cast<WORLDTEXTSTRING *>(text)->Render();
  }
}
