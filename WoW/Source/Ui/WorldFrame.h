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
BOOL UnitUpdateProc(DWORDLONG guid, LPVOID param);

struct CWorldClickEvent {
  UINT button;
};

struct CTerrainClickEvent {
  NTempest::C3Vector point;
  UINT               button;
};

struct CSpriteClickEvent {
  DWORDLONG          objectGUID;
  UINT               button;
  UINT               time;
  NTempest::C2Vector pos;
};

struct CObjectTrackEvent {
  CObjectTrackEvent() {
  }

  DWORDLONG object;
  DWORDLONG oldGUID;
  float     x;
  float     y;
};

class CGWorldFrame : public CSimpleFrame {
  friend BOOL UnitUpdateProc(DWORDLONG guid, LPVOID param);

 public:
  enum HIT_TYPE {
    HIT_NONE = 0,
    HIT_GROUND = 1,
    HIT_OBJECT = 2
  };

  struct HitTestResult {
    HitTestResult() : object(0), point(0.0f), distance(0.0f) {
    }

    DWORDLONG          object;
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
  virtual BOOL OnLayerTrackUpdate(const CMouseEvent &evt);
  virtual void OnFrameRender(CRenderBatch *batch, UINT layer);
  virtual void OnLayerCursorExit();
  virtual BOOL OnLayerKeyDown(CKeyEvent &evt);
  virtual BOOL OnLayerKeyUp(CKeyEvent &evt);
  virtual BOOL OnLayerMouseDown(CMouseEvent &evt);
  virtual BOOL OnLayerMouseUp(CMouseEvent &evt);
  virtual BOOL OnLayerMouseWheel(CMouseEvent &evt);
  virtual BOOL OnLayerMouseMoveRelative(CMouseEvent &evt);

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

  DWORD GetUpdateTimeStamp() {
    return m_updateTimeStamp;
  }

  void               SetCameraTarget(CGObject_C *target);
  void               UpdateObject(CGObject_C *object, DWORD status);
  void               AddModelToScene(CGObject_C *object, HMODEL__ *model);
  void               SetPlayerFadeCameraValue(BYTE value);
  void               SetSpriteClickButtons(UINT buttons);
  void               SetTerrainClickButtons(UINT buttons);
  BOOL               PerformDefaultAction(MOUSEBUTTON button, UINT timestamp);
  DWORDLONG          GetObjectUnderMouse();
  BOOL               TogglePlayerRender();
  int                SetPlayerRender(int state);
  void               OnMouseModeNormal();
  void               OnMouseModeRelative();
  static CGCamera   *GetActiveCamera();
  static void        GetCameraPosition(NTempest::C3Vector *position);
  static void        GetCameraFacing(NTempest::C3Vector *position);
  static void        RegisterObjectFadeoutModel(CGObject_C *object, HTEXCOMPONENT__ *texture, BYTE startAlpha);
  void               SetNamePlateUpdate();
  void               RefreshPlayerAlpha();
  NTempest::C2Vector GetScreenCoordinates(const NTempest::C3Vector &point);
  NTempest::C2Vector GetScreenCoordinates(const NTempest::C3Vector &point, const NTempest::C44Matrix &matrix, int clip, int worldPositionSpecified);

 protected:
  DWORDLONG FindClosestModel(const NTempest::C3Vector &a, const NTempest::C3Vector &b, UINT hitFilter, float *hitDist);
  float     GetSkyProgress();

  CGWorldFrame(CSimpleFrame *parent);

 private:
  UINT          SphereTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector, UINT hitFilter);
  UINT          VolumeTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector);
  UINT          GeometryTestModels(const NTempest::C3Vector &aVector, const NTempest::C3Vector &bVector);
  void          ReduceToClosestModel();
  CModelRecord *HigherPriorityModel(CModelRecord *a, CModelRecord *b);
  BOOL          IsLegalSelection(CModelRecord *record, UINT hitFilter);
  BOOL          IsUnitLegalSelection(const CGUnit_C *unit, UINT hitFilter);
  void          MoveToFreeList(CModelRecord *record);
  void          MoveToFreeList(LISTPTR(CModelRecord) objList);
  HIT_TYPE      HitTest(const NTempest::C3Vector &a, const NTempest::C3Vector &b, UINT hitFilter, HitTestResult *hitTestResult);
  HIT_TYPE      HitTestPoint(float x, float y, HitTestResult *hitTestResult);
  BOOL          SendObjectTrackEvent(DWORDLONG guid, float x, float y);
  BOOL          SendUnitFadeEvent(DWORDLONG guid);
  void          OnLayerTrackTerrain(const HitTestResult &hitTestResult);
  void          OnLayerTrackObject(const HitTestResult &hitTestResult, float x, float y);
  void          CursorTrackUnit(CGUnit_C *unit);
  void          CursorTrackObject(CGGameObject_C *gameObject);
  void          HideObstructingModels(float maxDist);
  UINT          GetHitTestFilterFlags() const;

 protected:
  void UpdateDayNightInfo(float elapsedSec);
  void UpdatePlayerAlpha(float elapsedSeconds);
  void HandleUnitFade(int nowTracking, int immediateFade);
  void UnitUpdate();
  void OnWorldUpdate();
  void OnWorldRender();

 public:
  static void RenderWorld(LPVOID param);

 private:
  BOOL GetLineSegment(float x, float y, NTempest::C3Vector *a, NTempest::C3Vector *b);

  LISTDECL(CModelRecord, m_models);
  LISTDECL(CModelRecord, m_filteredModels);
  LISTDECL(CModelRecord, m_freeModels);
  UINT                m_spriteButtons;
  UINT                m_terrainButtons;
  DWORDLONG           m_lastUnitFade;
  DWORDLONG           m_lastObjectTrack;
  float               m_lastUpdateElapsedSec;
  float               m_skyAnimDuration;
  UINT                m_renderPlayer : 1;
  UINT                m_freeLookMode : 1;
  NTempest::C44Matrix m_worldMatrix;

  static CGWorldFrame *s_currentWorldFrame;

  UINT  m_flags;
  float m_elapsedSec;
  char  m_lastKey[780][32];

 protected:
  CGCamera *m_camera;

 private:
  DWORD m_updateTimeStamp;

 protected:
  PLAYERFADEMODE m_playerFadeMode;
  int            m_playerAlpha;
  int            m_cameraAlpha;
  int            m_cameraAlphaChanged;
};

#endif
