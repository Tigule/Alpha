#include <Base/Handle.h>
#include <Event/EvtApi.h>
#include <Gx/Gx.h>
#include <Os/OsTime.h>
#include <Scrn/Scrn.h>
#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/cimvector.h>

static NTempest::C3Vector normal(0.0f, 0.0f, 1.0f);
static NTempest::C3Vector position[4] = {
    NTempest::C3Vector(0.0f, 0.0f, 0.0f), NTempest::C3Vector(1.0f, 0.0f, 0.0f), NTempest::C3Vector(0.0f, 1.0f, 0.0f),
    NTempest::C3Vector(1.0f, 1.0f, 0.0f)
};
static unsigned short     indices[4] = {0, 1, 2, 3};
static NTempest::C2Vector texCoord[4] = {
    NTempest::C2Vector(0.0f, 1.0f), NTempest::C2Vector(1.0f, 1.0f), NTempest::C2Vector(0.0f, 0.0f), NTempest::C2Vector(1.0f, 0.0f)
};

static unsigned int  s_fadingScreenEnabled;
static unsigned int  s_drawingFadingScreen;
static HLAYER__     *s_fadingScreenLayer;
static int           s_fadingMode;
static float         s_fadingTime;
static unsigned long s_fadingStart;
static unsigned int  s_fadingComplete;
static void(__fastcall *s_fadedCallback)(void *);
static void   *s_fadedCallbackParam;
static CGxTex *s_textureHandle;

void __fastcall FadingScreenPaint(void *param, const RECTF *rect, const RECTF *visibleRect, float alpha);

int __fastcall EatEvent(const void *data, void *param) {
  return 0;
}

void __fastcall RegisterHandlers() {
  EventRegisterEx(EVENT_ID_CHAR, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_KEYDOWN, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_KEYUP, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_KEYDOWN_REPEATING, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_MOUSEDOWN, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_MOUSEMOVE, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_MOUSEUP, EatEvent, 0, 8.0f);
}

void __fastcall UnregisterHandlers() {
  EventUnregister(EVENT_ID_CHAR, EatEvent);
  EventUnregister(EVENT_ID_KEYDOWN, EatEvent);
  EventUnregister(EVENT_ID_KEYDOWN_REPEATING, EatEvent);
  EventUnregister(EVENT_ID_KEYUP, EatEvent);
  EventUnregister(EVENT_ID_MOUSEDOWN, EatEvent);
  EventUnregister(EVENT_ID_MOUSEMOVE, EatEvent);
  EventUnregister(EVENT_ID_MOUSEUP, EatEvent);
}

void __fastcall FadingScreenCleanup() {
  HandleClose((HOBJECT)s_fadingScreenLayer);
  GxTexDestroy(s_textureHandle);
  UnregisterHandlers();
  s_fadingScreenEnabled = 0;
  s_drawingFadingScreen = 0;
}

void __fastcall FadingScreenPaint(void *, const RECTF *, const RECTF *, float) {
  float               elapsed = 0.0f;
  NTempest::CImVector color(0xFFFFFFFF);
  unsigned int        fadeComplete = 0;

  if (s_fadingMode) {
    if (s_drawingFadingScreen) {
      elapsed = static_cast<float>(OsGetAsyncTimeMs() - s_fadingStart) * 0.001f;
    } else {
      s_fadingStart = OsGetAsyncTimeMs();
    }

    if (elapsed >= s_fadingTime) {
      fadeComplete = 1;
      color.a = s_fadingMode == 2 ? 255 : 0;
    } else {
      unsigned char fadeAlpha = static_cast<unsigned char>(elapsed / s_fadingTime * 255.0f);
      color.a = s_fadingMode == 2 ? fadeAlpha : static_cast<unsigned char>(255 - fadeAlpha);
    }
  }

  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Fog, 0);
  GxRsSet(GxRs_Blend, GxBlend_Alpha);
  GxVertexShaderSelect(GxVS_PassThru);
  GxPrimLockVertexPtrs(4, position, sizeof(NTempest::C3Vector), &normal, 0, &color, 0, 0, 0, texCoord, sizeof(NTempest::C2Vector), 0, 0);
  GxRsSet(GxRs_Texture0, s_textureHandle);
  GxPrimDrawElements(GxPrim_TriangleStrip, 4, indices);
  GxPrimUnlockVertexPtrs();
  GxRsPop();

  s_drawingFadingScreen = 1;
  if (s_fadingComplete) {
    if (s_fadingMode == 1) {
      FadingScreenCleanup();
    }

    s_fadingMode = 0;
    s_fadingComplete = 0;
    if (s_fadedCallback) {
      s_fadedCallback(s_fadedCallbackParam);
    }
  } else {
    s_fadingComplete = fadeComplete;
  }
}

void __fastcall EnableFadingScreen(float fadeTime, void(__fastcall *fadedCallback)(void *), void *param) {
  RECTF rect;

  if (s_fadingScreenEnabled) {
    FadingScreenCleanup();
  }

  CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
  GxTexCreate(8, 8, GxTex_Argb8888, flags, 0, GxuUpdateSingleColorTexture, s_textureHandle);

  rect.left = 0.0f;
  rect.bottom = 0.0f;
  rect.right = 1.0f;
  rect.top = 1.0f;
  ScrnLayerCreate(&rect, 10.0f, fadeTime == 0.0f ? 6 : 7, 0, FadingScreenPaint, &s_fadingScreenLayer);
  RegisterHandlers();

  s_fadedCallback = fadedCallback;
  s_fadedCallbackParam = param;
  s_fadingScreenEnabled = 1;
  s_drawingFadingScreen = 0;
  s_fadingMode = 2;
  s_fadingTime = fadeTime;
  s_fadingComplete = 0;
}
