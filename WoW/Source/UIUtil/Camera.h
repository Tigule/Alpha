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

  void             MakeRelativeTo(DWORDLONG guid);
  const DWORDLONG &GetTarget() const {
    return m_target;
  }
  void SetTarget(CGObject_C *target);
  void SetPositionAndTarget(const NTempest::C3Vector &position, const NTempest::C3Vector &target);
  void SetPositionAndFacing(const NTempest::C3Vector &position, const NTempest::C3Vector &facing);
  BOOL SetModelCamera(LPCSTR modelFile, const NTempest::C3Vector &origin, float facing, int (*ModelCameraFinished)(LPVOID), LPVOID param);
  void ClearModelCamera();
  void ResetModelCamera();
  void SetupWorldProjection(const NTempest::CRect &projectionRect);
  void AddShake(int shake, const NTempest::C3Vector &position);
  void AddShake(CGCameraShakeType shakeType, CGCameraDir direction, float amplitude, float frequency, float duration, float phase, float coefficient);
  void ToggleFreeLook();
  void EnableFreeLook();
  void DisableFreeLook(int sticky);
  virtual NTempest::C3Vector Forward() const;
  virtual NTempest::C3Vector Right() const;
  virtual NTempest::C3Vector Up() const;
  void                       SyncFreeLookFacing();
  void                       UpdateFreeLookFacing(float dx, float dy);
  void                       CreateViewFromParams(int view, float dist, float pitch, float yaw);
  void                       CreateViewFromCamera(int view);
  void                       SetView(int newView);
  void                       CycleView();
  void                       NextView();
  void                       PreviousView();
  void                       ResetView(int view);
  void                       ZoomIn(float distance, DWORD timestamp);
  void                       ZoomOut(float distance, DWORD timestamp);
  void                       StartMotion(CGCameraMotion move, DWORD timestamp, DWORD timeout);
  void                       StopMotion(CGCameraMotion move, DWORD timestamp);
  void                       SetPositionAndTargetWithRoll(const NTempest::C3Vector &position, const NTempest::C3Vector &target, float roll);
  NTempest::C3Vector         Target() const;
  NTempest::C3Vector         Facing() const;
  int                        InFreeLookMode() const;
  int                        GetView() const;
  static BOOL                UpdateCallback(LPCVOID, LPVOID param);

 private:
  friend class CGWorldFrame;
  friend class CGInputControl;
  friend class CGUnit_C;

  BOOL                FinishLoadingModel();
  BOOL                FinishLoadingTarget(CGObject_C *target);
  int                 CompletedAngle() const;
  void                SetViewFlags(int flags);
  NTempest::C33Matrix ParentToWorld() const;
  static BOOL         CCommand_CameraClip(LPCSTR command, LPCSTR arguments);
  void                SetTargetFadeValue(BYTE value);
  void                SetModeNormal();
  void                SetModeFreeLook();
  void                CalcThirdPerson(CGObject_C *target, DWORD timestamp);
  void                CalcFirstPerson(CGObject_C *target, DWORD timestamp);
  void                ClampAngles();
  float               GetSmoothedYawAngle(float yaw, int moving);
  float               GetSmoothedHeight(float z, int moving);
  void                PerformTerrainTilt(DWORD timestamp, NTempest::C3Vector position, float facing, int moving, int turning, int updateOnly);
  void                SetDesiredDistance(float desiredDistance, DWORD timestamp);
  void                SetDesiredDistanceOverTime(float desiredDistance, float motionTime, DWORD timestamp);
  void                SetDesiredPitchAngle(float desiredAngle, float delay, DWORD timestamp);
  void                SetDesiredPitchAngleOverTime(float desiredAngle, float motionTime, DWORD timestamp);
  void                SetDesiredYawAngle(float desiredAngle, float delay, DWORD timestamp);
  void                SetDesiredYawAngleOverTime(float desiredAngle, float motionTime, DWORD timestamp);
  void                SetSmoothingAngle(float smoothingAngle, DWORD timestamp, int quickly);
  float               GetCameraDistance(float cameraDist, const NTempest::C3Vector &targetPosition);
  float               CollideCameraWithWorld(const NTempest::C3Vector &targetPosition);
  void                UpdateMotion(DWORD timestamp);
  void                CalcModelCamera(DWORD timestamp);
  void                RunShakes();
  void                CheckUnderwater();
  HMODEL__           *m_model;
  HCAMERA__          *m_modelCamera;
  NTempest::C34Matrix m_modelMatrix;
  DWORDLONG           m_target;
  float               m_targetOffsetZ;
  int                 m_flags;
  DWORDLONG           m_relativeTo;
  struct {
    float dist;
    float pitch;
    float yaw;
  } m_views[5];
  float              m_distance;
  float              m_yaw;
  float              m_pitch;
  float              m_roll;
  float              m_yawOffset;
  float              m_yawFreelookStart;
  DWORD              m_motionMask;
  DWORD              m_motionStart[6];
  DWORD              m_motionStop[6];
  DWORD              m_motionTimeout[6];
  NTempest::C3Vector m_lastTarget;
  float              m_savedTargetZ;
  float              m_lastFacing;
  DWORD              m_lastDeltaZ;
  float              m_smoothingAngle;
  DWORD              m_zoomSmoothingTimestamp;
  float              m_zoomTime;
  float              m_desiredDistance;
  float              m_previousDistance;
  DWORD              m_pitchSmoothingTimestamp;
  float              m_pitchTime;
  float              m_desiredPitch;
  float              m_previousPitch;
  DWORD              m_yawSmoothingTimestamp;
  float              m_yawTime;
  float              m_desiredYaw;
  float              m_previousYaw;
  int                m_cycleDirection;
  UINT               m_savedLiquid;
  LISTDECL(CameraShake, m_shakes);

  static int s_clipCamera;
};

void CameraInitialize();
void CameraRegisterScriptFunctions();
void CameraUnregisterScriptFunctions();
void CameraDestroy();

#endif
