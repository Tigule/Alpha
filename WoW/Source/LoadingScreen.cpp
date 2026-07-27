#include "Client.h"

#include <Base/Handle.h>
#include <Event/EvtApi.h>
#include <FrameXML/FrameXML.h>
#include <Gx/Gx.h>
#include <Images/blit.h>
#include <Scrn/Scrn.h>
#include <Services/Texture.h>
#include <Tempest/c2vector.h>
#include <Tempest/c3vector.h>
#include <Tempest/c44matrix.h>

#ifdef LoadImage
#undef LoadImage
#endif

enum TEXTURETYPE {
  TEXTURE_BACKGROUND = 0,
  TEXTURE_PROGRESS = 1,
  TEXTURE_BORDER = 2,
  TEXTURETYPES = 3
};

struct TEXTUREINFO {
  const char   *name;
  unsigned char scaleWithProgress;
  float         centerX;
  float         centerY;
  float         width;
  float         height;
  EGxBlend      blend;
};

static const TEXTUREINFO s_textureInfo[TEXTURETYPES] = {
    {                      "Interface\\Glues\\loading", 0, 0.5f,   0.5f,   1.0f,   1.0f, GxBlend_Alpha},
    {  "Interface\\Glues\\LoadingBar\\Loading-BarFill", 1, 0.5f, 0.075f, 0.525f, 0.025f, GxBlend_Alpha},
    {"Interface\\Glues\\LoadingBar\\Loading-BarBorder", 0, 0.5f, 0.075f,   0.6f,  0.05f, GxBlend_Alpha}
};

static const unsigned short s_indices[4] = {0, 1, 2, 3};
static unsigned int         s_textureFormat[TEXTURETYPES];
static int                  s_worldLoaded;
static int                  s_xmlTotal;
static HOBJECT              s_loadingScreenLayer;
static float                s_progress;
static MipBits             *s_mipBits[TEXTURETYPES];
static CGxTex              *s_textureHandles[TEXTURETYPES];
static int                  s_xmlLoaded;
static bool                 s_loadingScreenEnabled;

static void FrameXMLProgressCallback(int loaded, int total);
static void UpdateProgressBar();
static void LoadingScreenPaint(void *param, const RECTF *rect, const RECTF *visibleRect, float alpha);
static void TextureCallback(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
);

void DisableLoadingScreen();

static void UpdateProgress() {
  float progress = 0.0f;

  if (s_xmlTotal) {
    progress = static_cast<float>(s_xmlLoaded) / static_cast<float>(s_xmlTotal) * 0.75f;
  }

  if (s_worldLoaded) {
    progress += 0.25f;
  }

  s_progress = min(max(progress, 0.0f), 1.0f);
}

static void UpdateProgressBar() {
  float minX;
  float maxX;
  float minY;
  float maxY;
  float minZ;
  float maxZ;

  UpdateProgress();
  GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);
  GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);

  NTempest::C44Matrix identity;
  NTempest::C44Matrix orthoProj;
  RECTF               dummy2;
  RECTF               dummy3;
  GxuXformCreateOrtho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 500.0f, orthoProj);
  GxXformSetView(identity);
  GxXformSetProjection(orthoProj);
  LoadingScreenPaint(0, &dummy2, &dummy3, 0.0f);
  GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
  GxScenePresent(3);
}

static void FrameXMLProgressCallback(int loaded, int total) {
  s_xmlLoaded = loaded;
  s_xmlTotal = total;
  UpdateProgressBar();
}

static void InitializeProgressBar() {
  s_xmlLoaded = 0;
  s_xmlTotal = 1;
  s_worldLoaded = 0;
  s_progress = 0.0f;
  FrameXML_RegisterLoadProgressCallback(FrameXMLProgressCallback);
}

static void CleanupProgressBar() {
  FrameXML_RegisterLoadProgressCallback(0);
}

static int EatEvent(const void *data, void *param) {
  return 0;
}

static void RegisterHandlers() {
  EventRegisterEx(EVENT_ID_CHAR, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_KEYDOWN, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_KEYUP, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_KEYDOWN_REPEATING, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_MOUSEDOWN, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_MOUSEUP, EatEvent, 0, 8.0f);
  EventRegisterEx(EVENT_ID_MOUSEMOVE, EatEvent, 0, 8.0f);
}

static void UnregisterHandlers() {
  EventUnregister(EVENT_ID_CHAR, EatEvent);
  EventUnregister(EVENT_ID_KEYDOWN, EatEvent);
  EventUnregister(EVENT_ID_KEYDOWN_REPEATING, EatEvent);
  EventUnregister(EVENT_ID_KEYUP, EatEvent);
  EventUnregister(EVENT_ID_MOUSEDOWN, EatEvent);
  EventUnregister(EVENT_ID_MOUSEUP, EatEvent);
  EventUnregister(EVENT_ID_MOUSEMOVE, EatEvent);
}

static void LoadingScreenPaint(void *, const RECTF *, const RECTF *, float) {
  static const NTempest::C3Vector normal(0.0f, 0.0f, 1.0f);
  static const NTempest::C2Vector texCoord[4] = {
      NTempest::C2Vector(0.0f, 1.0f), NTempest::C2Vector(1.0f, 1.0f), NTempest::C2Vector(0.0f, 0.0f), NTempest::C2Vector(1.0f, 0.0f)
  };

  GxRsPush();
  GxRsSet(GxRs_Lighting, 0);
  GxRsSet(GxRs_Fog, 0);
  GxVertexShaderSelect(GxVS_PassThru);

  for (unsigned int image = 0; image < TEXTURETYPES; ++image) {
    if (s_textureHandles[image]) {
      const TEXTUREINFO &info = s_textureInfo[image];
      GxRsSet(GxRs_Blend, info.blend);

      NTempest::C3Vector position[4];
      float              halfWidth = info.width * 0.5f;
      float              left = info.centerX - halfWidth;
      float              halfHeight = info.height * 0.5f;
      position[0].x = left;
      position[2].x = left;
      position[0].y = info.centerY - halfHeight;
      position[1].y = position[0].y;
      position[1].x = info.centerX + halfWidth;
      position[3].x = position[1].x;
      position[2].y = info.centerY + halfHeight;
      position[3].y = position[2].y;

      if (info.scaleWithProgress) {
        position[1].x = s_progress * info.width + left;
        position[3].x = position[1].x;
      }

      position[0].z = 0.0f;
      position[1].z = 0.0f;
      position[2].z = 0.0f;
      position[3].z = 0.0f;
      GxPrimLockVertexPtrs(4, position, sizeof(NTempest::C3Vector), &normal, 0, 0, 0, 0, 0, texCoord, sizeof(NTempest::C2Vector), 0, 0);
      GxRsSet(GxRs_Texture0, s_textureHandles[image]);
      GxPrimDrawElements(GxPrim_TriangleStrip, 4, s_indices);
      GxPrimUnlockVertexPtrs();
    }
  }

  GxRsPop();
}

static void TextureCallback(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  unsigned int image = reinterpret_cast<unsigned int>(userArg);
  MipBits     *mipBits = s_mipBits[image];

  ASSERT(mipBits);

  switch (cmd) {
    case GxTex_Lock:
      ASSERT(mipBits != 0);
      break;

    case GxTex_Latch:
      texelStrideInBytes = CalcRowStride(GxGetBlitFormat(static_cast<EGxTexFormat>(s_textureFormat[image])), w);
      texels = mipBits->mip[mipLevel];
      break;

    case GxTex_Unlock:
      if (mipBits) {
        TextureFreeMippedImg(mipBits);
      }
      break;
  }
}

static void LoadImage(TEXTURETYPE image) {
  unsigned int width;
  unsigned int height;
  int          isOpaque;

  s_mipBits[image] = TextureLoadImage(s_textureInfo[image].name, &width, &height, &s_textureFormat[image], &isOpaque, 0, 0);

  if (s_mipBits[image]) {
    CGxTexFlags flags(GxTex_Linear, 0, 0, 0, 0, 0, 1);
    GxTexCreate(
        width, height, static_cast<EGxTexFormat>(s_textureFormat[image]), flags, reinterpret_cast<void *>(static_cast<unsigned int>(image)),
        TextureCallback, s_textureHandles[image]
    );
    ASSERT(s_textureHandles[image]);
  }
}

void EnableLoadingScreen() {
  RECTF rect;

  DisableLoadingScreen();

  for (unsigned int image = 0; image < TEXTURETYPES; ++image) {
    LoadImage(static_cast<TEXTURETYPE>(image));
  }

  rect.left = 0.0f;
  rect.bottom = 0.0f;
  rect.right = 1.0f;
  rect.top = 1.0f;
  ScrnLayerCreate(&rect, 9.0f, 6, 0, LoadingScreenPaint, reinterpret_cast<HLAYER__ **>(&s_loadingScreenLayer));
  RegisterHandlers();
  InitializeProgressBar();
  s_loadingScreenEnabled = true;
}

bool DrawingLoadingScreen() {
  return s_loadingScreenEnabled;
}

void DisableLoadingScreen() {
  unsigned int index;

  if (!s_loadingScreenEnabled) {
    return;
  }

  HandleClose(s_loadingScreenLayer);
  for (index = 0; index < TEXTURETYPES; ++index) {
    if (s_textureHandles[index]) {
      GxTexDestroy(s_textureHandles[index]);
    }
    s_textureHandles[index] = 0;
  }

  UnregisterHandlers();
  CleanupProgressBar();
  s_loadingScreenEnabled = false;
}

void LoadingScreenRegisterWorldLoaded() {
  s_worldLoaded = 1;
  UpdateProgressBar();
}
