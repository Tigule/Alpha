#ifndef WOW_SOURCE_UI_WORLDFRAME_H
#define WOW_SOURCE_UI_WORLDFRAME_H

#include "Frame/CSimpleFrame.h"
#include "UIUtil/Camera.h"

#include "Tempest/c44matrix.h"

#include <Event/EvtApi.h>

#include <stpl.h>

struct CModelRecord;
struct HTEXCOMPONENT__;
class CGObject_C;
class CGUnit_C;
class CGGameObject_C;
int __fastcall UnitUpdateProc(unsigned __int64 guid, void *param);

struct CWorldClickEvent {
  unsigned int button;
};

struct CTerrainClickEvent {
  NTempest::C3Vector point;
  unsigned int       button;
};

struct CSpriteClickEvent {
  unsigned __int64   objectGUID;
  unsigned int       button;
  unsigned int       time;
  NTempest::C2Vector pos;
};

struct CObjectTrackEvent {
  unsigned __int64 object;
  unsigned __int64 oldGUID;
  float            x;
  float            y;
};

class CGWorldFrame : public CSimpleFrame {
  friend int __fastcall UnitUpdateProc(unsigned __int64 guid, void *param);

 public:
  enum HIT_TYPE {
    HIT_NONE = 0,
    HIT_TERRAIN = 1,
    HIT_SPRITE = 2
  };

  struct HitTestResult {
    HitTestResult() : object(0), point(0.0f), distance(0.0f) {
    }

    unsigned __int64   object;
    NTempest::C3Vector point;
    float              distance;
  };

  enum PLAYERFADEMODE {
    PLAYER_FADE_NONE = 0,
    PLAYER_FADE_OUT = 1,
    PLAYER_FADE_IN = 2
  };

  virtual ~CGWorldFrame();
  virtual void OnLayerUpdate(float elapsedSec);
  virtual int  OnLayerTrackUpdate(const CMouseEvent &evt);
  virtual void OnFrameRender(CRenderBatch *batch, unsigned int layer);
  virtual void OnLayerCursorExit();
  virtual int  OnLayerKeyDown(CKeyEvent &evt);
  virtual int  OnLayerKeyUp(CKeyEvent &evt);
  virtual int  OnLayerMouseDown(CMouseEvent &evt);
  virtual int  OnLayerMouseUp(CMouseEvent &evt);
  virtual int  OnLayerMouseWheel(CMouseEvent &evt);
  virtual int  OnLayerMouseMoveRelative(CMouseEvent &evt);

  static CSimpleFrame *__fastcall Create(CSimpleFrame *parent) {
    return NEW(CGWorldFrame)(parent);
  }

  static CGWorldFrame *GetActive() {
    return s_currentWorldFrame;
  }

  CGCamera *Camera() {
    FATALASSERT(m_camera);
    return m_camera;
  }

  float GetElapsedSec() {
    return m_elapsedSec;
  }

  NTempest::C44Matrix GetCurrentWorldMatrix() {
    return m_worldMatrix;
  }

  unsigned long GetUpdateTimeStamp() {
    return m_updateTimeStamp;
  }

  void                        SetCameraTarget(CGObject_C *target);
  void                        UpdateObject(CGObject_C *object, unsigned long status);
  void                        AddModelToScene(CGObject_C *object, HMODEL__ *model);
  void                        SetPlayerFadeCameraValue(unsigned int value);
  void                        SetSpriteClickButtons(unsigned int buttons);
  void                        SetTerrainClickButtons(unsigned int buttons);
  int                         PerformDefaultAction(MOUSEBUTTON button, unsigned int timestamp);
  unsigned __int64            GetObjectUnderMouse();
  int                         TogglePlayerRender();
  int                         SetPlayerRender(int state);
  void                        OnMouseModeNormal();
  void                        OnMouseModeRelative();
  static CGCamera *__fastcall GetActiveCamera();
  static void __fastcall      GetCameraPosition(NTempest::C3Vector *position);
  static void __fastcall      GetCameraFacing(NTempest::C3Vector *position);
  static void __fastcall      RegisterObjectFadeoutModel(CGObject_C *object, HTEXCOMPONENT__ *texture, unsigned char startAlpha);
  void                        SetNamePlateUpdate();
  void                        RefreshPlayerAlpha();
  NTempest::C2Vector          GetScreenCoordinates(const NTempest::C3Vector &point);
  NTempest::C2Vector          GetScreenCoordinates(
      const NTempest::C3Vector &point,
      const NTempest::C44Matrix &matrix,
      int clip,
      int worldPositionSpecified
  );

 protected:
  unsigned __int64 FindClosestModel(NTempest::C3Vector &a, NTempest::C3Vector &b, unsigned int hitFilter, float *hitDist);
  float            GetSkyProgress();

 private:
  CGWorldFrame(CSimpleFrame *parent);

  unsigned int           SphereTestModels(NTempest::C3Vector &aVector, NTempest::C3Vector &bVector, unsigned int hitFilter);
  unsigned int           VolumeTestModels(NTempest::C3Vector &aVector, NTempest::C3Vector &bVector);
  unsigned int           GeometryTestModels(NTempest::C3Vector &aVector, NTempest::C3Vector &bVector);
  void                   ReduceToClosestModel();
  CModelRecord          *HigherPriorityModel(CModelRecord *a, CModelRecord *b);
  int                    IsLegalSelection(CModelRecord *record, unsigned int hitFilter);
  int                    IsUnitLegalSelection(CGUnit_C *unit, unsigned int hitFilter);
  void                   MoveToFreeList(CModelRecord *record);
  void                   MoveToFreeList(TSList<CModelRecord, TSGetLink<CModelRecord> > *objList);
  HIT_TYPE               HitTest(NTempest::C3Vector &a, NTempest::C3Vector &b, unsigned int hitFilter, HitTestResult *hitTestResult);
  HIT_TYPE               HitTestPoint(float x, float y, HitTestResult *hitTestResult);
  int                    SendObjectTrackEvent(unsigned __int64 guid, float x, float y);
  int                    SendUnitFadeEvent(unsigned __int64 guid);
  void                   OnLayerTrackTerrain(HitTestResult &hitTestResult);
  void                   OnLayerTrackObject(HitTestResult &hitTestResult, float x, float y);
  void                   CursorTrackUnit(CGUnit_C *unit);
  void                   CursorTrackObject(CGGameObject_C *gameObject);
  void                   HideObstructingModels(float maxDist);
  unsigned int           GetHitTestFilterFlags();
  void                   UpdateDayNightInfo(float elapsedSec);
  void                   UpdatePlayerAlpha(float elapsedSeconds);
  void                   HandleUnitFade(int nowTracking, int immediateFade);
  void                   UnitUpdate();
  void                   OnWorldUpdate();
  void                   OnWorldRender();
  static void __fastcall RenderWorld(void *param);
  int                    GetLineSegment(float x, float y, NTempest::C3Vector *a, NTempest::C3Vector *b);

  TSList<CModelRecord, TSGetLink<CModelRecord> > m_models;
  TSList<CModelRecord, TSGetLink<CModelRecord> > m_filteredModels;
  TSList<CModelRecord, TSGetLink<CModelRecord> > m_freeModels;
  unsigned int                                   m_spriteButtons;
  unsigned int                                   m_terrainButtons;
  unsigned __int64                               m_lastUnitFade;
  unsigned __int64                               m_lastObjectTrack;
  float                                          m_lastUpdateElapsedSec;
  float                                          m_skyAnimDuration;
  unsigned int                                   m_renderPlayer : 1;
  unsigned int                                   m_freeLookMode : 1;
  NTempest::C44Matrix                            m_worldMatrix;

  static CGWorldFrame *s_currentWorldFrame;

  unsigned int m_flags;
  float        m_elapsedSec;
  char         m_lastKey[780][32];

 protected:
  CGCamera      *m_camera;
  unsigned long  m_updateTimeStamp;
  PLAYERFADEMODE m_playerFadeMode;
  int            m_playerAlpha;
  int            m_cameraAlpha;
  int            m_cameraAlphaChanged;
};

#endif
