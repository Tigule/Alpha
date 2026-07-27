#ifndef WOW_SOURCE_UIUTIL_CAMERA_H
#define WOW_SOURCE_UIUTIL_CAMERA_H

#include "UIUtil/CSimpleCamera.h"

#include "Tempest/c34matrix.h"

#include <stpl.h>

struct CameraShake;
struct HCAMERA__;
struct HMODEL__;
class CGObject_C;
class CGInputControl;
class CGUnit_C;

enum CGCameraMotion {
  CAMERA_MOVE_IN = 0,
  CAMERA_MOVE_OUT = 1,
  CAMERA_MOVE_RIGHT = 2,
  CAMERA_MOVE_LEFT = 3,
  CAMERA_MOVE_UP = 4,
  CAMERA_MOVE_DOWN = 5,
  NUM_CAMERA_MOTIONS = 6
};

enum CGCameraShakeType {
  CAMSHAKE_SINE = 0,
  CAMSHAKE_DECAYED_SINE = 1,
  NUM_CAMERA_SHAKETYPES = 2
};

enum CGCameraDir {
  CAMERA_FORWARD = 0,
  CAMERA_RIGHT = 1,
  CAMERA_UP = 2,
  NUM_CAMERA_DIRECTIONS = 3
};

class CGCamera : public CSimpleCamera {
 public:
  CGCamera();
  ~CGCamera();

  void MakeRelativeTo(unsigned __int64 guid);
  const unsigned __int64 &GetTarget() const {
    return m_target;
  }
  void SetTarget(CGObject_C *target);
  void SetPositionAndTarget(const NTempest::C3Vector &position, const NTempest::C3Vector &target);
  void SetPositionAndFacing(const NTempest::C3Vector &position, const NTempest::C3Vector &facing);
  int  SetModelCamera(
      const char *modelFile, const NTempest::C3Vector &origin, float facing, int(__fastcall *ModelCameraFinished)(void *), void *param
  );
  void ClearModelCamera();
  void ResetModelCamera();
  void SetupWorldProjection(const NTempest::CRect &projectionRect);
  void AddShake(int shake, const NTempest::C3Vector &position);
  void AddShake(CGCameraShakeType shakeType, CGCameraDir direction, float amplitude, float frequency, float duration, float phase, float coefficient);
  void ToggleFreeLook();
  void EnableFreeLook();
  void DisableFreeLook(int sticky);
  virtual NTempest::C3Vector Forward();
  virtual NTempest::C3Vector Right();
  virtual NTempest::C3Vector Up();
  void                       SyncFreeLookFacing();
  void                       UpdateFreeLookFacing(float dx, float dy);
  void                       CreateViewFromParams(int view, float dist, float pitch, float yaw);
  void                       CreateViewFromCamera(int view);
  void                       SetView(int newView);
  void                       CycleView();
  void                       NextView();
  void                       PreviousView();
  void                       ResetView(int view);
  void                       ZoomIn(float distance, unsigned long timestamp);
  void                       ZoomOut(float distance, unsigned long timestamp);
  void                       StartMotion(CGCameraMotion move, unsigned long timestamp, unsigned long timeout);
  void                       StopMotion(CGCameraMotion move, unsigned long timestamp);

 private:
  friend class CGWorldFrame;
  friend class CGInputControl;
  friend class CGUnit_C;

  int                   FinishLoadingModel();
  int                   FinishLoadingTarget(CGObject_C *target);
  NTempest::C33Matrix   ParentToWorld();
  static int __fastcall CCommand_CameraClip(const char *command, const char *arguments);
  void                  SetTargetFadeValue(unsigned char value);
  void                  SetModeNormal();
  void                  SetModeFreeLook();
  static int __fastcall UpdateCallback(const void *__formal, void *param);
  void                  CalcThirdPerson(CGObject_C *target, unsigned long timestamp);
  void                  CalcFirstPerson(CGObject_C *target, unsigned long timestamp);
  void                  ClampAngles();
  float                 GetSmoothedYawAngle(float yaw, int moving);
  float                 GetSmoothedHeight(float z, int moving);
  void                  PerformTerrainTilt(unsigned long timestamp, NTempest::C3Vector position, float facing, int moving, int turning, int updateOnly);
  void                  SetDesiredDistance(float desiredDistance, unsigned long timestamp);
  void                  SetDesiredDistanceOverTime(float desiredDistance, float motionTime, unsigned long timestamp);
  void                  SetDesiredPitchAngle(float desiredAngle, float delay, unsigned long timestamp);
  void                  SetDesiredPitchAngleOverTime(float desiredAngle, float motionTime, unsigned long timestamp);
  void                  SetDesiredYawAngle(float desiredAngle, float delay, unsigned long timestamp);
  void                  SetDesiredYawAngleOverTime(float desiredAngle, float motionTime, unsigned long timestamp);
  void                  SetSmoothingAngle(float smoothingAngle, unsigned long timestamp, int quickly);
  float                 GetCameraDistance(float cameraDist, const NTempest::C3Vector &targetPosition);
  float                 CollideCameraWithWorld(const NTempest::C3Vector &targetPosition);
  void                  UpdateMotion(unsigned long timestamp);
  void                  CalcModelCamera(unsigned long timestamp);
  void                  SetPositionAndTargetWithRoll(const NTempest::C3Vector &position, const NTempest::C3Vector &target, float roll);
  void                  RunShakes();
  void                  CheckUnderwater();
  HMODEL__             *m_model;
  HCAMERA__            *m_modelCamera;
  NTempest::C34Matrix   m_modelMatrix;
  unsigned __int64      m_target;
  float                 m_targetOffsetZ;
  int                   m_flags;
  unsigned __int64      m_relativeTo;
  struct {
    float dist;
    float pitch;
    float yaw;
  } m_views[5];
  float                                        m_distance;
  float                                        m_yaw;
  float                                        m_pitch;
  float                                        m_roll;
  float                                        m_yawOffset;
  float                                        m_yawFreelookStart;
  unsigned long                                m_motionMask;
  unsigned long                                m_motionStart[6];
  unsigned long                                m_motionStop[6];
  unsigned long                                m_motionTimeout[6];
  NTempest::C3Vector                           m_lastTarget;
  float                                        m_savedTargetZ;
  float                                        m_lastFacing;
  unsigned long                                m_lastDeltaZ;
  float                                        m_smoothingAngle;
  unsigned long                                m_zoomSmoothingTimestamp;
  float                                        m_zoomTime;
  float                                        m_desiredDistance;
  float                                        m_previousDistance;
  unsigned long                                m_pitchSmoothingTimestamp;
  float                                        m_pitchTime;
  float                                        m_desiredPitch;
  float                                        m_previousPitch;
  unsigned long                                m_yawSmoothingTimestamp;
  float                                        m_yawTime;
  float                                        m_desiredYaw;
  float                                        m_previousYaw;
  int                                          m_cycleDirection;
  unsigned int                                 m_savedLiquid;
  TSList<CameraShake, TSGetLink<CameraShake> > m_shakes;

  static int s_clipCamera;
};

void __fastcall CameraInitialize();
void __fastcall CameraRegisterScriptFunctions();
void __fastcall CameraUnregisterScriptFunctions();
void __fastcall CameraDestroy();

#endif
