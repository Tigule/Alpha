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
int UnitUpdateProc(unsigned __int64 guid, void *param);

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
  CObjectTrackEvent() {
  }

  unsigned __int64 object;
  unsigned __int64 oldGUID;
  float            x;
  float            y;
};

class CGWorldFrame : public CSimpleFrame {
  friend int UnitUpdateProc(unsigned __int64 guid, void *param);

 public:
  enum HIT_TYPE {
    HIT_NONE = 0,
    HIT_GROUND = 1,
    HIT_OBJECT = 2
  };

  struct HitTestResult {
    HitTestResult() : object(0), point(0.0f), distance(0.0f) {
    }

    unsigned __int64   object;
    NTempest::C3Vector point;
    float              distance;
  };

  enum PLAYERFADEMODE {
    PLAYERFADE_NONE = 0,
    PLAYERFADE_OUT = 1,
    PLAYERFADE_IN = 2,
    NUM_PLAYERFADEMODES = 3
  };

  enum HIT_FILTER {
    HIT_TEST_NOTHING = 0,
    HIT_TEST_GROUND = 1,
    HIT_TEST_OBJECTS = 2,
    HIT_TEST_UNITS = 4,
    HIT_TEST_PLAYERS = 8,
    HIT_TEST_ME = 16,
    HIT_TEST_PARTY = 0x10000,
    HIT_TEST_FRIENDS = 0x20000,
    HIT_TEST_ENEMIES = 0x40000,
    HIT_TEST_LIVE = 0x100000,
    HIT_TEST_DEAD = 0x200000,
    HIT_TEST_ALL_OBJS_EXCEPT_ME = 14,
    HIT_TEST_ALL_OBJS = 30,
    HIT_TEST_ALL_EXCEPT_ME = 15,
    HIT_TEST_ALL = 31
  };

 protected:
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

 public:
  static CSimpleFrame *Create(CSimpleFrame *parent) {
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
  void                        SetPlayerFadeCameraValue(unsigned char value);
  void                        SetSpriteClickButtons(unsigned int buttons);
  void                        SetTerrainClickButtons(unsigned int buttons);
  int                         PerformDefaultAction(MOUSEBUTTON button, unsigned int timestamp);
  unsigned __int64            GetObjectUnderMouse();
  int                         TogglePlayerRender();
  int                         SetPlayerRender(int state);
  void                        OnMouseModeNormal();
  void                        OnMouseModeRelative();
  static CGCamera *GetActiveCamera();
  static void GetCameraPosition(NTempest::C3Vector *position);
  static void GetCameraFacing(NTempest::C3Vector *position);
  static void RegisterObjectFadeoutModel(CGObject_C *object, HTEXCOMPONENT__ *texture, unsigned char startAlpha);
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
  unsigned __int64 FindClosestModel(const NTempest::C3Vector &a, const NTempest::C3Vector &b, unsigned int hitFilter, float *hitDist);
  float            GetSkyProgress();

  CGWorldFrame(CSimpleFrame *parent);

 private:
  unsigned int           SphereTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector, unsigned int hitFilter);
  unsigned int           VolumeTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector);
  unsigned int           GeometryTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector);
  void                   ReduceToClosestModel();
  CModelRecord          *HigherPriorityModel(CModelRecord *a, CModelRecord *b);
  int                    IsLegalSelection(CModelRecord *record, unsigned int hitFilter);
  int                    IsUnitLegalSelection(const CGUnit_C *unit, unsigned int hitFilter);
  void                   MoveToFreeList(CModelRecord *record);
  void                   MoveToFreeList(LISTPTR(CModelRecord) objList);
  HIT_TYPE               HitTest(const NTempest::C3Vector &a, const NTempest::C3Vector &b, unsigned int hitFilter, HitTestResult *hitTestResult);
  HIT_TYPE               HitTestPoint(float x, float y, HitTestResult *hitTestResult);
  int                    SendObjectTrackEvent(unsigned __int64 guid, float x, float y);
  int                    SendUnitFadeEvent(unsigned __int64 guid);
  void                   OnLayerTrackTerrain(const HitTestResult &hitTestResult);
  void                   OnLayerTrackObject(const HitTestResult &hitTestResult, float x, float y);
  void                   CursorTrackUnit(CGUnit_C *unit);
  void                   CursorTrackObject(CGGameObject_C *gameObject);
  void                   HideObstructingModels(float maxDist);
  unsigned int           GetHitTestFilterFlags() const;

 protected:
  void                   UpdateDayNightInfo(float elapsedSec);
  void                   UpdatePlayerAlpha(float elapsedSeconds);
  void                   HandleUnitFade(int nowTracking, int immediateFade);
  void                   UnitUpdate();
  void                   OnWorldUpdate();
  void                   OnWorldRender();

 public:
  static void RenderWorld(void *param);

 private:
  int                    GetLineSegment(float x, float y, NTempest::C3Vector *a, NTempest::C3Vector *b);

  LISTDECL(CModelRecord, m_models);
  LISTDECL(CModelRecord, m_filteredModels);
  LISTDECL(CModelRecord, m_freeModels);
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

 private:
  unsigned long  m_updateTimeStamp;

 protected:
  PLAYERFADEMODE m_playerFadeMode;
  int            m_playerAlpha;
  int            m_cameraAlpha;
  int            m_cameraAlphaChanged;
};

#endif
