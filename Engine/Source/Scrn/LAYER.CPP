#include "Scrn.h"

#include <Base/Handle.h>
#include <Base/InstanceId.h>
#include <Event/EvtApi.h>
#include <Gx/Gx.h>
#include <Images/tga.h>
#include <Os/OsTime.h>
#include <Os/W32/OsFile.h>
#include <Tempest/c44matrix.h>
#include <Tempest/c3vector.h>
#include <Tempest/cimvector.h>

#include <new>

DECLARE_DERIVED_HANDLE(HLAYER, HOBJECT);

class CILayer : public CHandleObject {
 public:
  RECTF  rect;
  RECTF  visible;
  float  zorder;
  DWORD  flags;
  LPVOID param;
  void (*paintfunc)(LPVOID, const RECTF *, const RECTF *, float);
  LINKDECLEX(CILayer, zorderlink);
};

static const RECTF defaultrect = {0.0f, 0.0f, 1.0f, 1.0f};
static float       s_elapsedSec;
static void (*s_captureScreen)(int);
static int s_ScrnDisable;
static LISTDECLEX(CILayer, zorderlink, s_zorderlist);

static BOOL CaptureScreen() {
  char                       path[0x104];
  char                       timestamp[0x104];
  UINT                       w;
  UINT                       h;
  const NTempest::CImVector *pixels;

  GxDevReadScreenShot(w, h, pixels);

  if (pixels) {
    OsCreateDirectory("Screenshots", 0);
    OsGetTimeStamp(timestamp, sizeof(timestamp));
    SStrPrintf(path, sizeof(path), "%s\\WoWScrnShot_%s.tga", "Screenshots", timestamp);
    CTgaFile tga;
    tga.SetImage(pixels, w, h, 0x20, 8, 1, 0);
    tga.Write(path);
  }

  return pixels != 0;
}

static BOOL OnIdle(const EVENT_DATA_IDLE *data, LPVOID) {
  s_elapsedSec += data->elapsedSec;
  return 1;
}

static BOOL OnPaint(LPCVOID, LPVOID) {
  CILayer *nextLayer;
  HSRGN    rgn;
  RECTF    baseRect;
  float    minX;
  float    maxX;
  float    minY;
  float    maxY;
  float    minZ;
  float    maxZ;

  if (s_ScrnDisable || !GxCapsIsWindowVisible()) {
    return 1;
  }

  SRgnCreate(&rgn, 0);
  baseRect.left = 0.0f;
  baseRect.bottom = 0.0f;
  baseRect.right = 1.0f;
  baseRect.top = 1.0f;
  SRgnCombineRectf(rgn, &baseRect, 0, 2);

  ITERATELISTREVERSE(CILayer, s_zorderlist, layer) {
    SRgnGetBoundingRectf(rgn, &layer->visible);
    layer->visible.left = max(layer->visible.left, layer->rect.left);
    layer->visible.bottom = max(layer->visible.bottom, layer->rect.bottom);
    layer->visible.right = min(layer->visible.right, layer->rect.right);
    layer->visible.top = min(layer->visible.top, layer->rect.top);

    if (!(layer->flags & 1)) {
      SRgnCombineRectf(rgn, &layer->rect, 0, 4);
    }
  }

  SRgnDelete(rgn);
  GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);

  for (layer = s_zorderlist.Head(); (int)layer > 0 ? ((nextLayer = s_zorderlist.RawNext(layer)), 1) : 0; layer = nextLayer) {

    if (layer->visible.right <= layer->visible.left || layer->visible.top <= layer->visible.bottom) {
      continue;
    }

    if (layer->flags & 4) {
      GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    } else {
      GxXformSetViewport(layer->visible.left, layer->visible.right, layer->visible.bottom, layer->visible.top, 0.0f, 1.0f);
    }

    if (layer->flags & 2) {
      NTempest::C44Matrix identity;
      NTempest::C44Matrix orthoProj;

      GxuXformCreateOrtho(layer->visible.left, layer->visible.right, layer->visible.bottom, layer->visible.top, 0.0f, 500.0f, orthoProj);
      GxXformSetView(identity);
      GxXformSetProjection(orthoProj);
    }

    layer->paintfunc(layer->param, &layer->rect, &layer->visible, s_elapsedSec);
  }

  GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);

  if (s_captureScreen) {
    GxDevTakeScreenShot();
    GxScenePresent(3);
    s_captureScreen(CaptureScreen());
    GxDevClearScreenShot();
    s_captureScreen = 0;
  } else {
    GxScenePresent(3);
  }

  s_elapsedSec = 0.0f;
  return 1;
}

void ILayerDestroy() {
  EventUnregister(EVENT_ID_IDLE, (EVENTHANDLER)OnIdle);
  EventUnregister(EVENT_ID_PAINT, OnPaint);
}

void ILayerInitialize() {
  EventRegister(EVENT_ID_IDLE, (EVENTHANDLER)OnIdle);
  EventRegister(EVENT_ID_PAINT, OnPaint);
}

void ILayerPaint() {
  OnPaint(0, 0);
}

void ScrnLayerCreate(
    const RECTF *rect,
    float        zorder,
    DWORD        flags,
    LPVOID       param,
    void (*paintfunc)(LPVOID, const RECTF *, const RECTF *, float),
    HLAYER__ **layer
) {
  CILayer *newLayer;
  CILayer *existingLayer;

  if (!rect) {
    rect = &defaultrect;
  }

  newLayer = NEWHANDLE(HLAYER, CILayer);
  newLayer->rect = *rect;
  newLayer->zorder = zorder;
  newLayer->flags = flags;
  newLayer->param = param;
  newLayer->paintfunc = paintfunc;

  existingLayer = s_zorderlist.Tail();

  while (existingLayer && existingLayer->zorder > zorder) {
    existingLayer = s_zorderlist.Prev(existingLayer);
  }

  s_zorderlist.LinkNode(newLayer, LIST_LINK_AFTER, existingLayer);
  *layer = CREATEHANDLE(HLAYER, newLayer);
}

void ScrnLayerSetRect(HLAYER__ *layer, const RECTF *rect) {
  ((CILayer *)layer)->rect = *rect;
}

UINT ScrnLayerGetFlags(HLAYER__ *layer) {
  return ((CILayer *)layer)->flags;
}

void ScrnLayerSetFlags(HLAYER__ *layer, UINT flags) {
  ((CILayer *)layer)->flags = flags;
}

void ScrnLayerDisable(int disable) {
  s_ScrnDisable = disable;
}

int ScrnLayerIsDisabled() {
  return s_ScrnDisable;
}

void ScrnScreenshot(void (*callback)(int)) {
  s_captureScreen = callback;
}
