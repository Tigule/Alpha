#include <Base/Base.h>
#include <Gx/Gx.h>
#include <MapDefs.h>
#include <WorldClient/World.h>
#include <WowConst.h>
#include <DayNight.h>

#include "WorldText.h"

#include "Console/ConsoleCommand.h"
#include "Console/ConsoleVar.h"
#include "Object/ObjectClient/Object_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "Ui/WorldFrame.h"
#include "WorldClient/WorldParam.h"

#include <Base/Coordinate.h>
#include <FrameScript/FrameScript.h>
#include <Gxu/IGxuFont.h>
#include <Services/TextBlock.h>
#include <Tempest/c4vector.h>
#include <Tempest/crect.h>

#include <math.h>

enum SCREENRECTGRIDS {
  SRECTGRID_NAMEPLATES = 0,
  SRECTGRID_WORLDTEXT = 1,
  NUM_SRECTGRIDS = 2
};
void SmartScreenRectGridPos(SCREENRECTGRIDS grid, NTempest::CRect &rect);

struct WORLDTEXTSTRING : public CHandleObject {
  WORLDTEXTSTRING()
      : worldTextType(NUM_WORLDTEXTTYPES), elapsedTime(0), totalTime(0), object(0), hidden(0), m_flags(0), string(0) {
    params.Defaults();
    savedStringText[0] = 0;
  }
  void Update(float elapsed, const NTempest::C44Matrix &matrix, const NTempest::C3Vector *basePosition);
  void Reset();
  void CalculateNewColor(UINT elapsed);
  void CalculateTextHeight(UINT elapsedTime);
  void CalculateNewPosition(
      const NTempest::C4Vector  &worldPosition,
      UINT                       elapsedTime,
      NTempest::C4Vector        &textPos,
      const NTempest::C44Matrix &matrix,
      int                        worldPositionSpecified
  );
  void UpdatePosition(const NTempest::C4Vector &worldPosition, UINT elapsedTime, NTempest::C4Vector &textPos);
  void UpdateStringHeight(float height);
  void RecreateString();
  void Hide(int hide);
  void Render() const;
  void InitTextFrame(LPCSTR text);

  WORLDTEXTTYPE         worldTextType;
  WORLDTEXTCREATEPARAMS params;
  UINT                  elapsedTime;
  UINT                  totalTime;
  DWORDLONG             object;
  float                 textWidth;
  float                 textHeight;
  float                 heightScale;
  float                 zOffset;
  LINKDECLEX(WORLDTEXTSTRING, link);
  int        hidden;
  UINT       m_flags;
  CGxString *string;
  float      savedStringHeight;
  char       savedStringText[64];

  virtual ~WORLDTEXTSTRING() {
    if (string) {
      GxuFontDestroyString(string);
    }
    string = 0;
  }
};

static LISTDECLEX(WORLDTEXTSTRING, link, s_textList);
static WORLDTEXTCREATEPARAMS s_worldTextParams[NUM_WORLDTEXTTYPES];
static HTEXTFONT             s_worldTextFontHandles[NUM_WORLDTEXTTYPES];
static CVar                 *s_fontHeightCVar;
static CVar                 *s_fontOutlineCVar;
static CVar                 *s_fontFadeInTime;
static CVar                 *s_fontFadeOutTime;
static CVar                 *s_fontFadeTotalTime;
static CVar                 *s_fontFadeAscendDistance;
static CVar                 *s_fontCharSpacing;
static CVar                 *s_fontConeAngle;

static struct {
  float startProgress;
  float endProgress;
  float heightScale;
} s_jumps[3] = {
    {-0.33f, 0.33f, 1.5f},
    { 0.33f,  0.5f, 0.2f},
    {  0.5f, 0.66f, 0.1f}
};
static const struct {
  float startProgress;
  float endProgress;
  float startHeightScale;
  float endHeightScaleScale;
} s_critHeights[3] = {
    {0.0f, 0.1f, 0.1f, 2.0f},
    {0.1f, 0.2f, 2.0f, 1.0f},
    {0.2f, 1.0f, 1.0f, 1.0f}
};
static const struct {
  float startProgress;
  float endProgress;
  float startAlpha;
  float endAlpha;
} s_critFades[3] = {
    {0.0f, 0.1f, 0.0f, 1.0f},
    {0.1f, 0.5f, 1.0f, 1.0f},
    {0.5f, 1.0f, 1.0f, 0.0f}
};
static struct {
  int   r;
  int   g;
  int   b;
  float startProgress;
  float endProgress;
} s_fontColors[1] = {
    {255, 0, 0, 0.0f, 0.33f}
};

static UINT const worldTextFlags[NUM_WORLDTEXTTYPES] = {0x48, 0x48, 0x10, 0x8, 0x48, 0, 0};

WORLDTEXTCREATEPARAMS::WORLDTEXTCREATEPARAMS() {
}

static int IntInterp(float progress, int start, int end) {
  return static_cast<int>(static_cast<double>(end - start) * progress + static_cast<double>(start));
}

static NTempest::CImVector ColorInterp(float range, NTempest::CImVector startColor, NTempest::CImVector endColor) {
  range = min(range, 1.0f);
  return NTempest::CImVector(
      255, static_cast<BYTE>(IntInterp(range, startColor.r, endColor.r)), static_cast<BYTE>(IntInterp(range, startColor.g, endColor.g)),
      static_cast<BYTE>(IntInterp(range, startColor.b, endColor.b))
  );
}

void WORLDTEXTSTRING::Hide(int hide) {
  hidden = hide;
}

void WORLDTEXTSTRING::Render() const {
  if (!hidden && string) {
    GxuFontRender(string);
  }
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

  elapsedTime += static_cast<UINT>(elapsed * 1000.0f);
  if (elapsedTime > totalTime && !(params.flags & 1)) {
    return;
  }

  CalculateNewColor(elapsedTime);
  CGObject_C        *obj = ClntObjMgrObjectPtr(object, __FILE__, __LINE__);
  int                worldPositionSpecified;
  NTempest::C4Vector worldPosition;
  if (obj) {
    obj->GetPosition();
    obj->GetScale();
    worldPositionSpecified = 1;
  } else {
    worldPosition = *basePosition;
    worldPosition.z += params.zOffset;
    worldPositionSpecified = 0;
  }

  NTempest::C4Vector newTextPosition;
  CalculateNewPosition(worldPosition, elapsedTime, newTextPosition, matrix, worldPositionSpecified);
  if ((params.flags & 1) &&
      (newTextPosition.y < 0.0f || newTextPosition.x < 0.0f || newTextPosition.x >= 1.0f || newTextPosition.y >= 1.0f))
  {
    return;
  }

  float screenx;
  float screeny;
  NDCToDDC(newTextPosition.x, newTextPosition.y, &screenx, &screeny);
  float halfWidth = textWidth * 0.5f;
  float borderX = halfWidth + params.border.x;
  float halfHeight = textHeight * 0.5f;
  float textBorderY = halfHeight + params.border.y;
  float x = min(max(screenx, borderX), 0.8f - borderX);
  float y = min(max(screeny, textBorderY), 0.6f - textBorderY);

  NTempest::CRect rect(y + halfHeight, x - halfWidth, y - halfHeight, halfWidth + x);
  SmartScreenRectGridPos(SRECTGRID_WORLDTEXT, rect);

  NTempest::C3Vector position(DDCToNDCWidth((rect.r + rect.l) * 0.5f) - textWidth * 0.5f, DDCToNDCHeight((rect.b + rect.t) * 0.5f), 0.0f);
  GxuFontSetStringPosition(string, position);
}

void WORLDTEXTSTRING::Reset() {
  elapsedTime = 0;
}

void WORLDTEXTSTRING::CalculateNewPosition(
    const NTempest::C4Vector  &worldPosition,
    UINT                       elapsedTime,
    NTempest::C4Vector        &textPos,
    const NTempest::C44Matrix &matrix,
    int                        worldPositionSpecified
) {
  UpdatePosition(worldPosition, elapsedTime, textPos);
  CGWorldFrame *worldFramePtr = CGWorldFrame::GetActive();
  ASSERT(worldFramePtr);
  textPos = worldFramePtr->GetScreenCoordinates(NTempest::C3Vector(textPos.x, textPos.y, textPos.z), matrix, 1, worldPositionSpecified);
  textPos.w = 1.0f;
}

void WORLDTEXTSTRING::UpdatePosition(const NTempest::C4Vector &worldPosition, UINT elapsedTime, NTempest::C4Vector &textPos) {
  textPos = worldPosition;
  if (!(params.flags & 1)) {
    float progress = static_cast<float>(elapsedTime > 1 ? elapsedTime : 1) / static_cast<float>(totalTime);
    float distance = params.ascendDistance;
    if (params.flags & 8) {
      UINT i;
      for (i = 0; i < 3; ++i) {
        if (progress >= s_jumps[i].startProgress && progress <= s_jumps[i].endProgress) {
          float range = s_jumps[i].endProgress - s_jumps[i].startProgress;
          float rangeProgress;
          if (range != 0.0f) {
            rangeProgress = (progress - s_jumps[i].startProgress) / range;
          } else {
            rangeProgress = 1.0f;
          }
          float curve = static_cast<float>(sin(rangeProgress * PI));
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

void WORLDTEXTSTRING::CalculateTextHeight(UINT elapsedTime) {
  float heightScale;
  elapsedTime = min(elapsedTime, params.totalTime);

  if (params.flags & 0x10) {
    float progress = static_cast<float>(elapsedTime) / static_cast<float>(params.totalTime);
    heightScale = 1.0f;
    for (UINT i = 0; i < 3; ++i) {
      ASSERT(s_critHeights[i].startProgress <= s_critHeights[i].endProgress);
      if (progress >= s_critHeights[i].startProgress && progress < s_critHeights[i].endProgress) {
        float range = (progress - s_critHeights[i].startProgress) / (s_critHeights[i].endProgress - s_critHeights[i].startProgress);
        heightScale = range * (s_critHeights[i].endHeightScaleScale - s_critHeights[i].startHeightScale) + s_critHeights[i].startHeightScale;
        break;
      }
    }
    heightScale *= params.endFontHeight;
  } else if (!(params.flags & 1) && elapsedTime < params.enlargeTime) {
    heightScale = static_cast<float>(elapsedTime) / static_cast<float>(params.enlargeTime) * (params.endFontHeight - params.startFontHeight) +
             params.startFontHeight;
  } else if (!(params.flags & 1) && elapsedTime >= params.shrinkTime && elapsedTime < totalTime) {
    UINT duration = totalTime - params.shrinkTime;
    heightScale = static_cast<float>(duration - (elapsedTime - params.shrinkTime)) / static_cast<float>(duration) *
                 (params.endFontHeight - params.startFontHeight) +
             params.startFontHeight;
  } else {
    heightScale = params.endFontHeight;
  }

  UpdateStringHeight(max(heightScale, 0.001f));
}

void WORLDTEXTSTRING::RecreateString() {
  ASSERT(worldTextType < NUM_WORLDTEXTTYPES);
  if (string) {
    GxuFontDestroyString(string);
  }
  string = 0;

  if ((m_flags & 0x80) && savedStringText[0] && s_worldTextFontHandles[worldTextType]) {
    CGxFont *font = TextBlockGetFontPtr(s_worldTextFontHandles[worldTextType]);
    float    stringHeight = DDCToNDCHeight(savedStringHeight);
    GxuFontGetTextExtent(font, savedStringText, SStrLen(savedStringText), stringHeight, &textWidth, 0.0f, 0);
    textHeight = GxuFontGetWrappedTextHeight(font, savedStringText, stringHeight, textWidth, 0.0f, 0);
    GxuFontCreateString(
        font, savedStringText, stringHeight, NTempest::C3Vector(0.0f), textWidth, textHeight, 0.0f, string, GxVJ_Bottom, GxHJ_Left, 0,
        params.fontColor, 0.0f
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

void WORLDTEXTSTRING::CalculateNewColor(UINT elapsed) {
  ASSERT(string);
  BYTE alpha = 255;
  BYTE shadowAlpha = params.shadowColor.a;
  elapsedTime = min(elapsedTime, params.totalTime);
  float totalProgress = static_cast<float>(elapsedTime) / static_cast<float>(params.totalTime);

  if (params.flags & 0x10) {
    float alphaScale = 1.0f;
    for (UINT i = 0; i < 3; ++i) {
      ASSERT(s_critFades[i].startProgress <= s_critFades[i].endProgress);
      if (totalProgress >= s_critFades[i].startProgress && totalProgress < s_critFades[i].endProgress) {
        float range = (totalProgress - s_critFades[i].startProgress) / (s_critFades[i].endProgress - s_critFades[i].startProgress);
        alphaScale = range * (s_critFades[i].endAlpha - s_critFades[i].startAlpha) + s_critFades[i].startAlpha;
        break;
      }
    }
    alpha = min(255, static_cast<UINT>(alphaScale * 255.0f));
    shadowAlpha = min(shadowAlpha, static_cast<UINT>(shadowAlpha * alphaScale));
  } else if (!(params.flags & 1)) {
    if (elapsed < params.fadeInTime) {
      alpha = min(255, static_cast<UINT>(totalProgress * 255.0f));
      shadowAlpha = min(shadowAlpha, static_cast<UINT>(shadowAlpha * totalProgress));
    } else if (elapsed >= params.fadeOutTime) {
      UINT  fadeDuration = totalTime - params.fadeOutTime;
      float fade;
      if (fadeDuration) {
        fade = static_cast<float>(elapsed - params.fadeOutTime) / static_cast<float>(fadeDuration);
      } else {
        fade = 0.0f;
      }
      alpha = 255.0f - min(255.0f, max(0.0f, 255.0f * fade));
      shadowAlpha = 255.0f - min(static_cast<float>(shadowAlpha), max(0.0f, shadowAlpha * fade));
    }
  }

  NTempest::CImVector fontColor = params.fontColor;
  NTempest::CImVector shadowColor = params.shadowColor;
  if ((params.flags & 0x40) && totalProgress >= s_fontColors[0].startProgress && totalProgress < s_fontColors[0].endProgress) {
    float rangeProgress = (totalProgress - s_fontColors[0].startProgress) / (s_fontColors[0].endProgress - s_fontColors[0].startProgress);
    fontColor = ColorInterp(rangeProgress, fontColor, NTempest::CImVector(255, s_fontColors[0].r, s_fontColors[0].g, s_fontColors[0].b));
  }
  fontColor.a = alpha;
  shadowColor.a = shadowAlpha;
  GxuFontSetStringColor(string, fontColor);
  if (params.shadowOffset.x != 0.0f || params.shadowOffset.y != 0.0f) {
    GxuFontAddShadow(string, shadowColor, params.shadowOffset);
  }
}

void WORLDTEXTSTRING::InitTextFrame(LPCSTR text) {
  if (text && *text) {
    SStrPrintf(savedStringText, sizeof(savedStringText), "%s", text);
  }
}

static void InitConsoleVariables() {
  s_fontHeightCVar = CVar::Register("DamageFontHeight", "Height to use for drawing damage text", 0, "0.035", 0, DEFAULT, false, 0);
  s_fontOutlineCVar = CVar::Register("DamageFontOutline", "outline the damage text", 0, "1", 0, DEFAULT, false, 0);
  s_fontFadeInTime = CVar::Register("DamageFontFadeInTime", "Fadein time for damage text", 0, "200", 0, DEFAULT, false, 0);
  s_fontFadeOutTime = CVar::Register("DamageFontFadeInTime", "Fadein time for damage text", 0, "900", 0, DEFAULT, false, 0);
  s_fontFadeTotalTime = CVar::Register("DamageFontTotalTime", "Total time to draw damage text", 0, "1200", 0, DEFAULT, false, 0);
  s_fontFadeAscendDistance = CVar::Register("DamageFontAscendDistance", "world inches to make the damage text rise", 0, "7", 0, DEFAULT, false, 0);
  s_fontCharSpacing = CVar::Register("DamageFontCharSpacing", "character spacing for damage text", 0, "-0.001", 0, DEFAULT, false, 0);
  s_fontConeAngle = CVar::Register("DamageFontConeAngle", "", 0, "0", 0, DEFAULT, false, 0);
}

void WorldTextInitialize() {
  UINT i;
  for (i = 0; i < NUM_WORLDTEXTTYPES; ++i) {
    s_worldTextParams[i].Defaults();
  }

  InitConsoleVariables();

  float fontHeight = NDCToDDCHeight(s_fontHeightCVar->GetFloat());
  float critFontHeight = fontHeight * 1.5f;
  UINT  fontFlags[NUM_WORLDTEXTTYPES] = {4, 4, 4, 0, 4, 0, 0};
  float fontHeights[NUM_WORLDTEXTTYPES] = {fontHeight, fontHeight, critFontHeight, fontHeight, fontHeight, fontHeight, fontHeight};

  LPCSTR fontName = FrameScript_GetText("DAMAGE_TEXT_FONT", -1, GENDER_NOT_APPLICABLE);
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
    UINT  fadeInTime;
    UINT  fadeOutTime;
    UINT  totalTime;
    float ascendDist;
    float smallFontHeight;
    float largeFontontHeight;
    float heightScale;
    float heightOffset;
    UINT  enlargeTime;
    UINT  shrinkTime;
  } worldTextInfo[NUM_WORLDTEXTTYPES] = {
      {                                            0,                                            800,800,        ascendDistance, fontHeight,     fontHeight, 1.0f, 0.27777778f, 0, 0                                                                                                     },
      {                                            0,                                            800,                   800,        ascendDistance, fontHeight,     fontHeight, 1.0f, 0.27777778f, 0, 0},
      {                                            0,                                            800,                   800,                  0.0f,       0.0f, critFontHeight, 1.0f, 0.27777778f, 0, 0},
      {static_cast<UINT>(s_fontFadeInTime->GetInt()), static_cast<UINT>(s_fontFadeOutTime->GetInt()),
       static_cast<UINT>(s_fontFadeTotalTime->GetInt()),        ascendDistance, fontHeight,     fontHeight, 1.0f, 0.27777778f, 0, 0                                                                    },
      {                                            0,                                            800,                   800,        ascendDistance, fontHeight,     fontHeight, 1.0f, 0.27777778f, 0, 0},
      {                                          500,                                            400,                  6000, ascendDistance * 2.0f, fontHeight,     fontHeight, 1.0f,        0.0f, 0, 0},
      {                                            0,                          static_cast<UINT>(-1), static_cast<UINT>(-1),                  0.0f, fontHeight,     fontHeight, 1.0f, 0.27777778f, 0, 0}
  };

  for (i = 0; i < NUM_WORLDTEXTTYPES; ++i) {
    WORLDTEXTCREATEPARAMS &params = s_worldTextParams[i];
    params.ascendDistance = worldTextInfo[i].ascendDist;
    params.totalTime = worldTextInfo[i].totalTime;
    params.fadeInTime = worldTextInfo[i].fadeInTime;
    params.fadeOutTime = worldTextInfo[i].fadeOutTime;
    params.fontColor = colorArray[i];
    params.shadowOffset = NTempest::C2Vector(0.001f, -0.001f);
    params.shadowColor = NTempest::CImVector(127, 0, 0, 0);
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
  for (UINT i = 0; i < NUM_WORLDTEXTTYPES; ++i) {
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

void WorldTextGetColor(WORLDTEXTTYPE type, NTempest::CImVector *color) {
  FATALASSERT(type < NUM_WORLDTEXTTYPES);
  FATALASSERT(color);
  *color = s_worldTextParams[type].fontColor;
}

HWORLDTEXT WorldTextCreate(WORLDTEXTTYPE type, LPCSTR text, DWORDLONG object, const NTempest::CImVector *colorOverride) {
  FATALASSERT(type < NUM_WORLDTEXTTYPES);
  WORLDTEXTCREATEPARAMS *params = &s_worldTextParams[type];
  FATALASSERT(params);
  FATALASSERT(params->fadeInTime <= params->totalTime);
  FATALASSERT(params->fadeOutTime <= params->totalTime);

  WORLDTEXTSTRING *worldTextPtr = NEWHANDLE(HWORLDTEXT, WORLDTEXTSTRING);
  FATALASSERT(worldTextPtr);
  s_textList.LinkNode(worldTextPtr, LIST_TAIL, 0);
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
  return CREATEHANDLE(HWORLDTEXT, worldTextPtr);
}

void WorldTextUpdate(float elapsed, const NTempest::C44Matrix &matrix) {
  ITERATELIST(WORLDTEXTSTRING, s_textList, text) {
    if (text->object) {
      text->Update(elapsed, matrix, 0);
    }
  }
}

void WorldTextUpdate(HWORLDTEXT text, float elapsed, const NTempest::C44Matrix &matrix, const NTempest::C3Vector *position) {
  if (text) {
    reinterpret_cast<WORLDTEXTSTRING *>(text)->Update(elapsed, matrix, position);
  }
}

BOOL WorldTextIsTextDone(HWORLDTEXT handle) {
  WORLDTEXTSTRING *text = reinterpret_cast<WORLDTEXTSTRING *>(handle);
  if (text) {
    return text->elapsedTime > text->totalTime && !(text->params.flags & 1);
  }
  return 1;
}

void WorldTextShow(HWORLDTEXT text, int show) {
  if (text) {
    if (show) {
      reinterpret_cast<WORLDTEXTSTRING *>(text)->Hide(0);
    } else {
      reinterpret_cast<WORLDTEXTSTRING *>(text)->Hide(1);
    }
  }
}

void WorldTextRender(HWORLDTEXT text) {
  if (text) {
    reinterpret_cast<WORLDTEXTSTRING *>(text)->Render();
  }
}
