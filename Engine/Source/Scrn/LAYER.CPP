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

class CILayer : public CHandleObject {
public:
  RECTF rect;
  RECTF visible;
  float zorder;
  unsigned long flags;
  void *param;
  void (*paintfunc)(void *, const RECTF *, const RECTF *, float);
  LINKDECLEX(CILayer, zorderlink);
};

typedef char CILayerSizeCheck[sizeof(CILayer) == 0x40 ? 1 : -1];

static const RECTF defaultrect = {0.0f, 0.0f, 1.0f, 1.0f};
static float s_elapsedSec;
static void (*s_captureScreen)(int);
static int s_ScrnDisable;
static LISTDECLEX(CILayer, zorderlink, s_zorderlist);

static int CaptureScreen() {
  char path[0x104];
  char timestamp[0x104];
  CTgaFile tga;
  unsigned int w;
  unsigned int h;
  const NTempest::CImVector *pixels;

  GxDevReadScreenShot(w, h, pixels);

  if (pixels) {
    OsCreateDirectory("Screenshots", 0);
    OsGetTimeStamp(timestamp, sizeof(timestamp));
    SStrPrintf(path,
               sizeof(path),
               "%s\\WoWScrnShot_%s.tga",
               "Screenshots",
               timestamp);
    tga.SetImage(pixels, w, h, 0x20, 8, 1, 0);
    tga.Write(path);
    tga.Close();
  }

  return pixels != 0;
}

static int OnIdle(const EVENT_DATA_IDLE *data, void *__formal) {
  s_elapsedSec += data->elapsedSec;
  return 1;
}

static int OnPaint(const void *, void *) {
  CILayer *layer;
  CILayer *nextLayer;
  HSRGN rgn;
  RECTF baseRect;
  float minX;
  float maxX;
  float minY;
  float maxY;
  float minZ;
  float maxZ;

  if (s_ScrnDisable || !GxCapsIsWindowVisible()) {
    return 1;
  }

  SRgnCreate(&rgn, 0);
  baseRect.left = 0.0f;
  baseRect.bottom = 0.0f;
  baseRect.right = 1.0f;
  baseRect.top = 1.0f;
  SRgnCombineRectf(rgn, &baseRect, 0, 2);

  layer = s_zorderlist.Tail();

  while (layer) {
    CILayer *previousLayer;

    SRgnGetBoundingRectf(rgn, &layer->visible);
    layer->visible.left = layer->rect.left < layer->visible.left
                            ? layer->visible.left
                            : layer->rect.left;
    layer->visible.bottom = layer->rect.bottom < layer->visible.bottom
                              ? layer->visible.bottom
                              : layer->rect.bottom;
    layer->visible.right = layer->rect.right > layer->visible.right
                             ? layer->visible.right
                             : layer->rect.right;
    layer->visible.top = layer->rect.top > layer->visible.top
                           ? layer->visible.top
                           : layer->rect.top;

    if (!(layer->flags & 1)) {
      SRgnCombineRectf(rgn, &layer->rect, 0, 4);
    }

    previousLayer = s_zorderlist.Prev(layer);
    layer = previousLayer;
  }

  SRgnDelete(rgn);
  GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);

  layer = s_zorderlist.Head();
  while (layer) {
    nextLayer = s_zorderlist.Next(layer);

    if (!(layer->visible.right <= layer->visible.left) &&
        !(layer->visible.top <= layer->visible.bottom)) {
      if (layer->flags & 4) {
        GxXformSetViewport(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
      } else {
        GxXformSetViewport(layer->visible.left,
                          layer->visible.right,
                          layer->visible.bottom,
                          layer->visible.top,
                          0.0f,
                          1.0f);
      }

      if (layer->flags & 2) {
        NTempest::C44Matrix identity;
        NTempest::C44Matrix orthoProj;

        GxuXformCreateOrtho(layer->visible.left,
                            layer->visible.right,
                            layer->visible.bottom,
                            layer->visible.top,
                            0.0f,
                            500.0f,
                            orthoProj);
        GxXformSetView(identity);
        GxXformSetProjection(orthoProj);
      }

      layer->paintfunc(layer->param,
                       &layer->rect,
                       &layer->visible,
                       s_elapsedSec);
    }

    layer = nextLayer;
  }

  GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);

  if (s_captureScreen) {
    void (*callback)(int) = s_captureScreen;

    GxDevTakeScreenShot();
    GxScenePresent(3);
    callback(CaptureScreen());
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
    float zorder,
    unsigned long flags,
    void *param,
    void (*paintfunc)(void *, const RECTF *, const RECTF *, float),
    HLAYER__ **layer) {
  CILayer *newLayer;
  CILayer *existingLayer;
  void *memory;

  if (!rect) {
    rect = &defaultrect;
  }

  memory = SMemAlloc(sizeof(CILayer), "HLAYER", SERR_LINECODE_OBJECT, 0);
  newLayer = memory ? new (memory) CILayer : 0;
  newLayer->rect = *rect;
  newLayer->zorder = zorder;
  newLayer->flags = flags;
  newLayer->param = param;
  newLayer->paintfunc = paintfunc;

  existingLayer = s_zorderlist.Tail();

  while (existingLayer && existingLayer->zorder > zorder) {
    CILayer *previousLayer = s_zorderlist.Prev(existingLayer);
    existingLayer = previousLayer;
  }

  s_zorderlist.LinkNode(newLayer, LIST_LINK_AFTER, existingLayer);
  *layer = reinterpret_cast<HLAYER__ *>(
      HandleCreate(newLayer, "HLAYER"));
}

void ScrnLayerSetRect(HLAYER__ *layer, const RECTF *rect) {
  reinterpret_cast<CILayer *>(layer)->rect = *rect;
}

unsigned int ScrnLayerGetFlags(HLAYER__ *layer) {
  return reinterpret_cast<CILayer *>(layer)->flags;
}

void ScrnLayerSetFlags(HLAYER__ *layer, unsigned int flags) {
  reinterpret_cast<CILayer *>(layer)->flags = flags;
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
