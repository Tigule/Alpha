#include <Base/Base.h>
#include <Frame/CSimpleTop.h>
#include <Frame/CSimpleModel.h>
#include <WowConst.h>
#include "UIUtil/Camera.h"
#include "UIUtil/InputControl.h"
#include "UIUtil/Tooltip.h"
#include <MapDefs.h>
#include <WorldClient/World.h>
#include "Ui/GameUI.h"

#include "Camera.h"

#include "DB/DBClient/AutoCode/CameraShakesRec.h"
#include "SoundInterface/SoundInterface.h"
#include "Ui/WorldFrame.h"

#include <Console/ConsoleClient.h>
#include <Console/ConsoleVar.h>
#include <FrameScript/FrameScript.h>

static const float CAMERASHAKEMINDISTANCE = 9.0f;
static const float CAMERASHAKEMAXDISTANCE = 80.0f;
static const float CAMERASHAKEMINDISTANCESQUARED = CAMERASHAKEMINDISTANCE * CAMERASHAKEMINDISTANCE;
static const float CAMERASHAKEMAXDISTANCESQUARED = CAMERASHAKEMAXDISTANCE * CAMERASHAKEMAXDISTANCE;

#include "Object/ObjectClient/Object_C.h"
#include "Object/ObjectClient/GameObject_C.h"
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "UIUtil/InputControl.h"
#include "Ui/GameUI.h"
#include "WorldClient/World.h"
#include "WorldClient/Map.h"

#include <Model/IModel.h>
#include <Base/Coordinate.h>
#include <Base/Handle.h>
#include <Base/Status.h>
#include <Event/EvtApi.h>
#include <Os/OsTime.h>
#include <Services/AsyncFileRead.h>
#include <Services/DataMgr.h>
#include <Services/SysMessage.h>
#include <Tempest/caabox.h>
#include <Tempest/caasphere.h>
#include <storm.h>

#include <float.h>
#include <math.h>

#include <lua.h>

float OrganicSmooth(float from, float to, float progress);

class RangeList {
 public:
  RangeList(float min, float max) : m_numranges(1) {
    m_ranges[0].m_min = min;
    m_ranges[0].m_max = max;
  }

  void RemoveRange(float iMin, float iMax);

  int GetNumRanges() const {
    return m_numranges;
  }

  BOOL GetRange(int index, float &min, float &max) {
    min = 0.0f;
    max = 0.0f;
    if (index >= 0 && index < m_numranges) {
      min = m_ranges[index].m_min;
      max = m_ranges[index].m_max;
      return 1;
    }
    return 0;
  }

 protected:
  int m_numranges;

 public:
  struct range {
    float m_min;
    float m_max;
  };

 protected:
  range m_ranges[4];
};

void RangeList::RemoveRange(float iMin, float iMax) {
  for (;;) {
    int numranges = m_numranges;
    int i;
    for (i = 0; i < numranges; ++i) {
      if (iMax <= m_ranges[i].m_min) {
        return;
      }
      if (iMin >= m_ranges[i].m_max) {
        continue;
      }
      break;
    }

    if (i == numranges) {
      return;
    }

    if (iMin <= m_ranges[i].m_min) {
      if (iMax >= m_ranges[i].m_max) {
        iMin = m_ranges[i].m_max;
        --m_numranges;
        memcpy(&m_ranges[i], &m_ranges[i + 1], sizeof(range) * (m_numranges - i));
        if (iMin < iMax) {
          continue;
        }
        return;
      } else {
        m_ranges[i].m_min = iMax;
        return;
      }
    } else {
      if (iMax >= m_ranges[i].m_max) {
        float oldmax = m_ranges[i].m_max;
        m_ranges[i].m_max = iMin;
        iMin = oldmax;
        if (iMin < iMax) {
          continue;
        }
        return;
      } else {
        if (i < sizeof(m_ranges) / sizeof(m_ranges[0]) - 1) {
          if (i < m_numranges - 1) {
            memmove(&m_ranges[i + 2], &m_ranges[i + 1], sizeof(range) * (m_numranges - (i + 1)));
          }
          if (m_numranges < sizeof(m_ranges) / sizeof(m_ranges[0])) {
            ++m_numranges;
          }
        }
        if (i < sizeof(m_ranges) / sizeof(m_ranges[0])) {
          m_ranges[i + 1].m_min = iMax;
          m_ranges[i + 1].m_max = m_ranges[i].m_max;
        }
        m_ranges[i].m_max = iMin;
        return;
      }
    }
  }
}

NODEDECL(CameraShake) {
  CGCameraShakeType m_shakeType;
  CGCameraDir       m_direction;
  float             m_amplitude;
  float             m_frequency;
  float             m_duration;
  float             m_phase;
  float             m_coefficient;
  DWORD             m_timestamp;
};

void CGCamera::AddShake(
    CGCameraShakeType shakeType,
    CGCameraDir       direction,
    float             amplitude,
    float             frequency,
    float             duration,
    float             phase,
    float             coefficient
) {
  CameraShake *shake = m_shakes.NewNode(LIST_TAIL, 0, 0);
  if (shake) {
    shake->m_shakeType = shakeType;
    shake->m_direction = direction;
    shake->m_amplitude = amplitude;
    shake->m_frequency = frequency;
    shake->m_duration = duration;
    shake->m_phase = phase;
    shake->m_coefficient = coefficient;
    shake->m_timestamp = OsGetAsyncTimeMs();
  }
}

void CGCamera::AddShake(int shake, const NTempest::C3Vector &position) {
  const CameraShakesRec *rec = g_cameraShakesDB.GetRecord(shake);
  if (!rec) {
    return;
  }

  float squaredMag = (position - m_position).SquaredMag();
  if (squaredMag > CAMERASHAKEMAXDISTANCESQUARED) {
    return;
  }

  float amplitude = rec->m_amplitude / 36.0f;
  if (squaredMag > CAMERASHAKEMINDISTANCESQUARED) {
    amplitude *= static_cast<float>(pow(0.7f, (NTempest::CMath::sqrt_(squaredMag) - CAMERASHAKEMINDISTANCE) / CAMERASHAKEMINDISTANCE));
  }
  AddShake(
      static_cast<CGCameraShakeType>(rec->m_shakeType), static_cast<CGCameraDir>(rec->m_direction), amplitude, rec->m_frequency, rec->m_duration,
      rec->m_phase, rec->m_coefficient
  );
}

static CVar       *s_cameraFarZ;
static CVar       *s_cameraNearZ;
static CVar       *s_cameraFOV;
static const float FREE_LOOK_SPEED = 1.5707964f;
static CVar       *s_mouseInvertYaw;
static CVar       *s_mouseInvertPitch;
static CVar       *s_cameraSmooth;
static CVar       *s_cameraSmoothingTime;
static CVar       *s_cameraLinearSpeed;
static CVar       *s_cameraAngularSpeed;
static CVar       *s_cameraAngleA;
static CVar       *s_cameraDistanceA;
static CVar       *s_cameraAngleB;
static CVar       *s_cameraDistanceB;
static CVar       *s_cameraAngleC;
static CVar       *s_cameraDistanceC;
static CVar       *s_cameraAngleD;
static CVar       *s_cameraDistanceD;

int CGCamera::s_clipCamera = 1;

static const float TARGET_RADIUS = 0.8888889f;
static const float MAX_PITCH_ANGLE = 1.5533431f;
static const float MIN_PITCH_ANGLE = -0.34906584f;
static const float MAX_CAMERA_DIST = 15.0f;
static const float MIN_CAMERA_DIST = 0.0f;
static const float CAMERA_BUMP_DIST = 0.11111111f;
static const float m_maxSmoothZ = 2.7777777f;
static const float ROTATION_SPEED = 0.09f;
static const float MAX_DELTA = 0.2617994f;
static const DWORD DELTA_Z_FREQUENCY = 100;
static const float FOCAL_DISTANCE = 3.3333333f;
static const float WAIST_HEIGHT = 1.6666666f;
static const float minSmoothAngle = 0.17453292f;
static const float minAngle = -1.5533431f;
static const float fadeAngle = -1.1868238f;

static const float CAMERA_SMOOTH_TIME = 1.0f;
static const float CAMERA_SMOOTH_MAXIMUM_ANGLE = 0.61086524f;
static const float CAMERA_SMOOTH_MINIMUM_ANGLE = -0.52359879f;

static bool ValidateIsInRange(LPCSTR strValue, float min, float max) {
  float value = SStrToFloat(strValue);

  if (value >= min && value <= max) {
    return true;
  }

  ConsoleWriteA("Value out of range (%f - %f)\n", DEFAULT_COLOR, min, max);
  return false;
}

static bool ValidateCameraDistance(CVar *cvar, LPCSTR oldValue, LPCSTR newValue, LPVOID arg) {
  ASSERT(newValue);

  float min = s_cameraNearZ->GetFloat();
  return ValidateIsInRange(newValue, TARGET_RADIUS + min, 27.777779f);
}

static bool ValidateCameraAngle(CVar *cvar, LPCSTR oldValue, LPCSTR newValue, LPVOID arg) {
  ASSERT(newValue);

  return ValidateIsInRange(newValue, -90.0f, 90.0f);
}

static int Script_CameraZoomIn(lua_State *L) {
  FATALASSERT(CGInputControl::GetActive());
  DWORD timestamp = lua_isnumber(L, 1) ? static_cast<DWORD>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs();
  float distance = lua_isnumber(L, 2) ? static_cast<float>(lua_tonumber(L, 2)) : 1.0f;
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  FATALASSERT(worldFrame);
  worldFrame->Camera()->ZoomIn(distance, timestamp);
  return 0;
}

static int Script_CameraZoomOut(lua_State *L) {
  FATALASSERT(CGInputControl::GetActive());
  DWORD timestamp = lua_isnumber(L, 1) ? static_cast<DWORD>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs();
  float distance = lua_isnumber(L, 2) ? static_cast<float>(lua_tonumber(L, 2)) : 1.0f;
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  FATALASSERT(worldFrame);
  worldFrame->Camera()->ZoomOut(distance, timestamp);
  return 0;
}

static int Script_MoveViewStart(lua_State *L, CGCameraMotion motion) {
  FATALASSERT(CGInputControl::GetActive());
  DWORD timestamp = lua_isnumber(L, 1) ? static_cast<DWORD>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs();
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  FATALASSERT(worldFrame);
  worldFrame->Camera()->StartMotion(motion, timestamp, 0);
  return 0;
}

static int Script_MoveViewStop(lua_State *L, CGCameraMotion motion) {
  FATALASSERT(CGInputControl::GetActive());
  DWORD timestamp = lua_isnumber(L, 1) ? static_cast<DWORD>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs();
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  FATALASSERT(worldFrame);
  worldFrame->Camera()->StopMotion(motion, timestamp);
  return 0;
}

static int Script_MoveViewInStart(lua_State *L) {
  return Script_MoveViewStart(L, CAMERA_MOVE_IN);
}

static int Script_MoveViewInStop(lua_State *L) {
  return Script_MoveViewStop(L, CAMERA_MOVE_IN);
}

static int Script_MoveViewOutStart(lua_State *L) {
  return Script_MoveViewStart(L, CAMERA_MOVE_OUT);
}

static int Script_MoveViewOutStop(lua_State *L) {
  return Script_MoveViewStop(L, CAMERA_MOVE_OUT);
}

static int Script_MoveViewRightStart(lua_State *L) {
  return Script_MoveViewStart(L, CAMERA_MOVE_RIGHT);
}

static int Script_MoveViewRightStop(lua_State *L) {
  return Script_MoveViewStop(L, CAMERA_MOVE_RIGHT);
}

CGCamera::~CGCamera() {
  SetTarget(0);
  ClearModelCamera();
  ConsoleCommandUnregister("cameraClip");
}

static int Script_MoveViewLeftStart(lua_State *L) {
  return Script_MoveViewStart(L, CAMERA_MOVE_LEFT);
}

static int Script_MoveViewLeftStop(lua_State *L) {
  return Script_MoveViewStop(L, CAMERA_MOVE_LEFT);
}

static int Script_MoveViewUpStart(lua_State *L) {
  return Script_MoveViewStart(L, CAMERA_MOVE_UP);
}

static int Script_MoveViewUpStop(lua_State *L) {
  return Script_MoveViewStop(L, CAMERA_MOVE_UP);
}

static int Script_MoveViewDownStart(lua_State *L) {
  return Script_MoveViewStart(L, CAMERA_MOVE_DOWN);
}

static int Script_MoveViewDownStop(lua_State *L) {
  return Script_MoveViewStop(L, CAMERA_MOVE_DOWN);
}

static int Script_ToggleMouseMove(lua_State *) {
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  FATALASSERT(worldFrame);
  worldFrame->Camera()->ToggleFreeLook();
  return 0;
}

static int Script_SetView(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int view = static_cast<int>(lua_tonumber(L, 1));
    if (view > 0 && view <= 5) {
      CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
      FATALASSERT(worldFrame);
      worldFrame->Camera()->SetView(view - 1);
    }
  }
  return 0;
}

static int Script_SaveView(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int view = static_cast<int>(lua_tonumber(L, 1));
    if (view > 0 && view <= 5) {
      CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
      FATALASSERT(worldFrame);
      worldFrame->Camera()->CreateViewFromCamera(view - 1);
    }
  }
  return 0;
}

static int Script_ResetView(lua_State *L) {
  if (lua_isnumber(L, 1)) {
    int view = static_cast<int>(lua_tonumber(L, 1));
    if (view > 0 && view <= 5) {
      CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
      FATALASSERT(worldFrame);
      worldFrame->Camera()->ResetView(view - 1);
    }
  }
  return 0;
}

static int Script_NextView(lua_State *) {
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  FATALASSERT(worldFrame);
  worldFrame->Camera()->NextView();
  return 0;
}

static int Script_PrevView(lua_State *) {
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  FATALASSERT(worldFrame);
  worldFrame->Camera()->PreviousView();
  return 0;
}

void CameraInitialize() {
  s_cameraFarZ = CVar::Lookup("farclip");
  s_cameraNearZ = CVar::Lookup("nearclip");
  s_cameraFOV = CVar::Lookup("fov");

  s_mouseInvertYaw = CVar::Register("mouseInvertYaw", 0, 0, "0", 0, DEFAULT, false, 0);
  s_mouseInvertPitch = CVar::Register("mouseInvertPitch", 0, 0, "0", 0, DEFAULT, false, 0);
  s_cameraSmooth = CVar::Register("camerasmooth", 0, 0, "1", 0, DEFAULT, false, 0);
  s_cameraSmoothingTime = CVar::Register("cameraSmoothingRate", 0, 0, "0.5", 0, DEFAULT, false, 0);
  s_cameraLinearSpeed = CVar::Register("cameraLinearSpeed", 0, 0, "8.33", 0, DEFAULT, false, 0);
  s_cameraAngularSpeed = CVar::Register("cameraAngularSpeed", 0, 0, "180.0", 0, DEFAULT, false, 0);

  s_cameraAngleA = CVar::Register("cameraAngleA", 0, 0, "0", ValidateCameraAngle, DEFAULT, false, 0);
  s_cameraDistanceA = CVar::Register("cameraDistanceA", 0, 0, "5.55", ValidateCameraDistance, DEFAULT, false, 0);
  s_cameraAngleB = CVar::Register("cameraAngleB", 0, 0, "20", ValidateCameraAngle, DEFAULT, false, 0);
  s_cameraDistanceB = CVar::Register("cameraDistanceB", 0, 0, "5.55", ValidateCameraDistance, DEFAULT, false, 0);
  s_cameraAngleC = CVar::Register("cameraAngleC", 0, 0, "30", ValidateCameraAngle, DEFAULT, false, 0);
  s_cameraDistanceC = CVar::Register("cameraDistanceC", 0, 0, "13.88", ValidateCameraDistance, DEFAULT, false, 0);
  s_cameraAngleD = CVar::Register("cameraAngleD", 0, 0, "0", ValidateCameraAngle, DEFAULT, false, 0);
  s_cameraDistanceD = CVar::Register("cameraDistanceD", 0, 0, "13.88", ValidateCameraDistance, DEFAULT, false, 0);
}

static FrameScript_Method s_ScriptFunctions[] = {
    {      "CameraZoomIn",       Script_CameraZoomIn},
    {     "CameraZoomOut",      Script_CameraZoomOut},
    {   "MoveViewInStart",    Script_MoveViewInStart},
    {    "MoveViewInStop",     Script_MoveViewInStop},
    {  "MoveViewOutStart",   Script_MoveViewOutStart},
    {   "MoveViewOutStop",    Script_MoveViewOutStop},
    { "MoveViewLeftStart",  Script_MoveViewLeftStart},
    {  "MoveViewLeftStop",   Script_MoveViewLeftStop},
    {"MoveViewRightStart", Script_MoveViewRightStart},
    { "MoveViewRightStop",  Script_MoveViewRightStop},
    {   "MoveViewUpStart",    Script_MoveViewUpStart},
    {    "MoveViewUpStop",     Script_MoveViewUpStop},
    { "MoveViewDownStart",  Script_MoveViewDownStart},
    {  "MoveViewDownStop",   Script_MoveViewDownStop},
    {   "ToggleMouseMove",    Script_ToggleMouseMove},
    {           "SetView",            Script_SetView},
    {          "SaveView",           Script_SaveView},
    {         "ResetView",          Script_ResetView},
    {          "NextView",           Script_NextView},
    {          "PrevView",           Script_PrevView}
};

void CameraRegisterScriptFunctions() {
  for (UINT i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void CameraUnregisterScriptFunctions() {
  for (UINT i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}

void CameraDestroy() {
}

CGCamera::CGCamera()
    : CSimpleCamera(s_cameraNearZ->GetFloat(), s_cameraFarZ->GetFloat(), s_cameraFOV->GetFloat() * 0.017453292f),
      m_model(0),
      m_modelCamera(0),
      m_flags(0x11),
      m_relativeTo(0),
      m_distance(s_cameraDistanceA->GetFloat()),
      m_yaw(0.0f),
      m_pitch(0.0f),
      m_roll(0.0f),
      m_yawOffset(0.0f),
      m_yawFreelookStart(0.0f),
      m_motionMask(0),
      m_lastTarget(0.0f),
      m_lastFacing(0.0f),
      m_lastDeltaZ(0),
      m_smoothingAngle(0.0f),
      m_zoomSmoothingTimestamp(0),
      m_desiredDistance(m_distance),
      m_pitchSmoothingTimestamp(0),
      m_desiredPitch(0.0f),
      m_yawSmoothingTimestamp(0),
      m_desiredYaw(0.0f),
      m_cycleDirection(1) {
  memset(m_motionStart, 0, sizeof(m_motionStart));
  memset(m_motionStop, 0, sizeof(m_motionStop));
  memset(m_motionTimeout, 0, sizeof(m_motionTimeout));
  ResetView(-1);
  ConsoleCommandRegister("cameraClip", CCommand_CameraClip, DEBUG, 0);
  SetTarget(0);
}

BOOL CGCamera::FinishLoadingModel() {
  m_flags |= 0x40;
  m_modelCamera = ModelGetCamera(m_model, 0);
  if (!m_modelCamera) {
    HandleClose(m_model);
    m_model = 0;
    return 0;
  }

  ModelAnimateCameras(m_model, m_modelMatrix);
  NTempest::C3Vector position(0.0f);
  NTempest::C3Vector target(0.0f);
  DataMgrGetCoord(reinterpret_cast<HDATAMGR>(m_modelCamera), 7, &position);
  DataMgrGetCoord(reinterpret_cast<HDATAMGR>(m_modelCamera), 8, &target);
  float roll = DataMgrGetFloat(reinterpret_cast<HDATAMGR>(m_modelCamera), 5);
  SetPositionAndTargetWithRoll(position, target, roll);
  return 1;
}

void CGCamera::ClearModelCamera() {
  if (m_model) {
    m_flags &= ~0x40U;
    if (m_modelCamera) {
      HandleClose(m_modelCamera);
    }
    HandleClose(m_model);
    m_model = 0;
  }
}

BOOL CGCamera::FinishLoadingTarget(CGObject_C *target) {
  HMODEL model = target->m_model;
  if (!model || !ModelIsLoaded(model, 1)) {
    return 0;
  }

  m_flags |= 0x40;
  if (target->IsA(ID_UNIT)) {
    m_targetOffsetZ = static_cast<CGUnit_C *>(target)->m_move.GetCollisionBoxHeight() * 0.99f;
    NTempest::C3Vector position(0.0f);
    if (ModelGetModelSpacePivot(model, 0x15, &position) && position.z + 0.1388889f < m_targetOffsetZ) {
      m_targetOffsetZ = position.z + 0.1388889f;
    }
  } else if (target->IsA(ID_DYNAMICOBJECT)) {
    NTempest::CAaSphere bounds;
    ModelGetBounds(model, &bounds);
    if (bounds.r > 0.01f) {
      m_targetOffsetZ = bounds.r * 0.99f;
    } else {
      m_targetOffsetZ = 2.0f;
    }
  } else {
    m_targetOffsetZ = 2.0f;
  }
  return 1;
}

void CGCamera::SetTarget(CGObject_C *target) {
  m_savedTargetZ = 0.0f;
  if (target) {
    m_target = target->GetGUID();
    m_lastFacing = target->IsA(ID_UNIT) ? static_cast<CGUnit_C *>(target)->m_move.GetRawFacing() : target->GetFacing();
    if (target->m_model && ModelIsLoaded(target->m_model, 1)) {
      FinishLoadingTarget(target);
      UpdateCallback(0, this);
    } else {
      m_flags &= ~0x40;
    }
  } else {
    m_target = 0;
    while (CameraShake *shake = m_shakes.Head()) {
      m_shakes.DeleteNode(shake);
    }
  }
}

void CGCamera::SetPositionAndTargetWithRoll(const NTempest::C3Vector &position, const NTempest::C3Vector &target, float roll) {
  NTempest::C3Vector facing = target - position;
  float              magnitude = facing.Mag();
  if (NTempest::CMath::fnotequal_(magnitude, 0.0f)) {
    facing *= 1.0f / magnitude;
  }

  m_position = position;
  NTempest::C3Vector up(0.0f, NTempest::CMath::sin_(roll), NTempest::CMath::cos_(roll));
  SetFacing(facing, up);
}

void CGCamera::ClampAngles() {
  m_pitch = NTempest::CMath::clamp_(m_pitch, -1.5533431f, 1.5533431f);

  while (m_yaw < 0.0f) {
    m_yaw += 6.2831855f;
  }
  while (m_yaw > 6.2831855f) {
    m_yaw -= 6.2831855f;
  }
}

float CGCamera::GetSmoothedYawAngle(float yaw, int moving) {
  if (s_cameraSmooth->GetInt() && moving) {
    float deltaFacing = yaw - m_lastFacing;
    if (NTempest::CMath::fabs_(deltaFacing) > ROTATION_SPEED) {
      if (deltaFacing > PI) {
        deltaFacing -= 6.2831855f;
      } else if (deltaFacing < -PI) {
        deltaFacing += 6.2831855f;
      }

      if (NTempest::CMath::fabs_(deltaFacing) > MAX_DELTA) {
        if (deltaFacing < 0.0f) {
          deltaFacing += MAX_DELTA;
        } else {
          deltaFacing -= MAX_DELTA;
        }
        yaw = deltaFacing + m_lastFacing;
      } else if (deltaFacing < 0.0f) {
        yaw = m_lastFacing - ROTATION_SPEED;
      } else {
        yaw = ROTATION_SPEED + m_lastFacing;
      }

      if (yaw < 0.0f) {
        yaw += 6.2831855f;
      } else if (yaw > 6.2831855f) {
        yaw -= 6.2831855f;
      }
    }
  }

  m_lastFacing = yaw;
  return yaw;
}

float CGCamera::GetCameraDistance(float cameraDist, const NTempest::C3Vector &targetPosition) {
  int                boxClipped = 0;
  NTempest::C3Vector cameraPosition = targetPosition - Forward() * cameraDist;
  CWFrustum          cameraFrustum(cameraPosition, cameraPosition + Forward(), Up(), m_fov * 57.29578f, m_aspect, m_nearZ, m_farZ);

  float              boxLength = cameraDist - m_nearZ;
  NTempest::C3Vector boxExtent = Forward() * boxLength;
  NTempest::C3Vector boxPoints[8] = {
      cameraFrustum.Corner(0),
      cameraFrustum.Corner(1),
      cameraFrustum.Corner(2),
      cameraFrustum.Corner(3),
      cameraFrustum.Corner(0) + boxExtent,
      cameraFrustum.Corner(1) + boxExtent,
      cameraFrustum.Corner(2) + boxExtent,
      cameraFrustum.Corner(3) + boxExtent,
  };

  CWFrustum   boxFrustum(boxPoints);
  CWFacetData boxIntersect;
  CWorld::GetFacets(boxFrustum, &boxIntersect, 0x121);

  NTempest::C44Matrix xform;
  if (boxIntersect.facets.Count() && CWorld::NDCXform(boxFrustum, xform, false)) {
    RangeList boxRange(0.0f, 1.0f);
    UINT      count = boxIntersect.facets.Count();
    for (UINT i = 0; i < count; ++i) {
      NTempest::CFacet &facet = boxIntersect.facets[i];
      facet.vertices[0] = (facet.vertices[0] - boxFrustum.Corner(0)) * xform;
      facet.vertices[1] = (facet.vertices[1] - boxFrustum.Corner(0)) * xform;
      facet.vertices[2] = (facet.vertices[2] - boxFrustum.Corner(0)) * xform;

      NTempest::C3Vector **clipped_points;
      UINT                 clipped_count;
      if (CWorld::NDCClip(facet.vertices, 3, clipped_points, clipped_count)) {
        float maxZ = 1.17549435e-38f;
        float minZ = FLT_MAX;
        for (UINT j = 0; j < clipped_count; ++j) {
          if (clipped_points[j]->z < minZ) {
            minZ = clipped_points[j]->z;
          }
          if (clipped_points[j]->z > maxZ) {
            maxZ = clipped_points[j]->z;
          }
        }
        if (minZ < maxZ) {
          boxClipped = 1;
          boxRange.RemoveRange(minZ, maxZ);
        }
      }
    }

    if (boxClipped) {
      float normalizedCameraBump = CAMERA_BUMP_DIST / boxLength;
      float normalizedDist = 1.0f;
      for (i = 0; i < boxRange.GetNumRanges(); ++i) {
        float min;
        float max;
        boxRange.GetRange(i, min, max);
        if (max - min > normalizedCameraBump) {
          normalizedDist = min + normalizedCameraBump;
          break;
        }
      }
      cameraDist -= normalizedDist * boxLength;
    }
  }

  return cameraDist;
}

BOOL CGCamera::CCommand_CameraClip(LPCSTR command, LPCSTR arguments) {
  if (arguments && *arguments) {
    s_clipCamera = SStrToInt(arguments);
  } else {
    s_clipCamera = !s_clipCamera;
  }
  return 1;
}

float CGCamera::CollideCameraWithWorld(const NTempest::C3Vector &targetPosition) {
  float cameraDist = max(m_desiredDistance, m_distance);
  if (s_clipCamera) {
    float initialDist = cameraDist;
    if (cameraDist - m_nearZ > 0.00000095367432f) {
      NTempest::C3Vector cp;
      NTempest::C3Vector tp = targetPosition - Forward() * cameraDist;
      float              hitT = 1.0f;
      if (CWorld::Intersect(&targetPosition, &tp, 0.0f, &cp, &hitT, 0x151)) {
        cameraDist *= hitT;
      }

      if (cameraDist > 0.00000095367432f) {
        cameraDist = GetCameraDistance(cameraDist, targetPosition);
        if (NTempest::CMath::fnotequal_(initialDist, cameraDist)) {
          cameraDist -= CAMERA_BUMP_DIST;
          cameraDist = max(cameraDist, 0.0f);
        }
      }
    }

    if (cameraDist > m_distance) {
      return m_distance;
    }
  }

  return cameraDist;
}

void CGCamera::CalcThirdPerson(CGObject_C *target, DWORD timestamp) {
  VALIDATEBEGIN;
  VALIDATE(target);
  VALIDATEENDVOID;
  if (!((m_flags & 0x40) || FinishLoadingTarget(target))) {
    return;
  }

  CGUnit_C          *unit = target->IsA(ID_UNIT) ? static_cast<CGUnit_C *>(target) : 0;
  NTempest::C3Vector targetPosition = target->GetPosition();
  int                hasMoved = targetPosition != m_lastTarget;
  if (hasMoved) {
    m_lastTarget = targetPosition;
  }
  targetPosition.z += m_targetOffsetZ;
  m_savedTargetZ = 0.0f;

  float playerYaw;
  if (unit) {
    unit->UpdateSmoothFacing();
    playerYaw = unit->GetRawSmoothFacing();
    if (!unit->IsClientControlled()) {
      playerYaw = GetSmoothedYawAngle(playerYaw, hasMoved);
    }
  } else {
    playerYaw = target->GetFacing();
  }

  if (m_motionMask) {
    UpdateMotion(timestamp);
  }

  if (NTempest::CMath::fnotequal_(m_desiredDistance, m_distance)) {
    float timeFac = ((timestamp - m_zoomSmoothingTimestamp) * 0.001f) / m_zoomTime;
    if (timeFac >= 1.0f) {
      m_distance = m_desiredDistance;
    } else {
      m_distance = OrganicSmooth(m_previousDistance, m_desiredDistance, timeFac);
    }
  } else {
    m_distance = m_desiredDistance;
    m_zoomSmoothingTimestamp = 0;
  }

  if (!(m_flags & 8)) {
    if (NTempest::CMath::fnotequal_(m_desiredPitch, m_pitch)) {
      if (timestamp >= m_pitchSmoothingTimestamp) {
        float timeFac = ((timestamp - m_pitchSmoothingTimestamp) * 0.001f) / m_pitchTime;
        if (timeFac >= 1.0f) {
          m_pitch = m_desiredPitch;
        } else {
          m_pitch = OrganicSmooth(m_previousPitch, m_desiredPitch, timeFac);
        }
      }
    } else {
      m_flags |= 0x10;
      m_pitchSmoothingTimestamp = 0;
      m_pitch = m_desiredPitch;
    }

    if (NTempest::CMath::fnotequal_(m_desiredYaw, m_yawOffset)) {
      if (timestamp >= m_yawSmoothingTimestamp) {
        float timeFac = ((timestamp - m_yawSmoothingTimestamp) * 0.001f) / m_yawTime;
        if (timeFac >= 1.0f) {
          m_yawOffset = m_desiredYaw;
        } else {
          m_yawOffset = OrganicSmooth(m_previousYaw, m_desiredYaw, timeFac);
        }
      }
    } else {
      m_yawSmoothingTimestamp = 0;
      m_yawOffset = m_desiredYaw;
    }

    m_yaw = playerYaw + m_yawOffset;
    ClampAngles();
  }

  SetFacing(m_yaw, m_pitch, m_roll);
  float cameraDist = CollideCameraWithWorld(targetPosition);
  FATALASSERT(cameraDist >= 0.0f);
  m_position = targetPosition - Forward() * cameraDist;

  if (m_distance - cameraDist > CAMERA_BUMP_DIST) {
    m_zoomTime = 2.0f;
    m_zoomSmoothingTimestamp = timestamp;
    m_distance = CAMERA_BUMP_DIST + cameraDist + 0.00000095367432f;
    m_previousDistance = m_distance;
  }

  float fadeDistance = s_cameraDistanceA->GetFloat() * 0.33f;
  if (m_pitch < fadeAngle) {
    float pitchFactor = (minAngle - m_pitch) / (minAngle - fadeAngle);
    fadeDistance /= pitchFactor * pitchFactor;
  }

  cameraDist -= m_nearZ;
  if (cameraDist >= fadeDistance) {
    SetTargetFadeValue(255);
  } else if (cameraDist > 0.0027777778f) {
    BYTE fadeValue = static_cast<BYTE>(OrganicSmooth(0.0f, 255.0f, cameraDist / fadeDistance));
    SetTargetFadeValue(fadeValue);
  } else {
    SetTargetFadeValue(0);
    NTempest::CMath::sincos_(m_yaw, m_facing.a1, m_facing.a0);
  }

  CSimpleCamera::SetFacing(NTempest::C3Vector(m_facing.a0, m_facing.a1, m_facing.a2));
}

void CGCamera::CalcFirstPerson(CGObject_C *target, DWORD timestamp) {
  VALIDATEBEGIN;
  VALIDATE(target);
  VALIDATEENDVOID;

  if ((m_flags & 0x40) || FinishLoadingTarget(target)) {
    float yaw = target->GetFacing();
    m_position = target->GetPosition();
    m_position.z += m_targetOffsetZ;
    if (m_flags & 8) {
      SetFacing(m_yaw, m_pitch, m_roll);
    } else {
      SetFacing(yaw, m_pitch, 0.0f);
    }
    m_zoomSmoothingTimestamp = 0;
    m_pitchSmoothingTimestamp = 0;
    m_yawSmoothingTimestamp = 0;
  }
}

void CGCamera::CalcModelCamera(DWORD timestamp) {
  if ((m_flags & 0x40) || (ModelIsLoaded(m_model, 1) && FinishLoadingModel())) {
    if (ModelAdvanceTime(m_model)) {
      ModelAnimateCameras(m_model, m_modelMatrix);
      NTempest::C3Vector position(0.0f);
      NTempest::C3Vector target(0.0f);
      DataMgrGetCoord(reinterpret_cast<HDATAMGR>(m_modelCamera), 7, &position);
      DataMgrGetCoord(reinterpret_cast<HDATAMGR>(m_modelCamera), 8, &target);
      float roll = DataMgrGetFloat(reinterpret_cast<HDATAMGR>(m_modelCamera), 5);
      SetPositionAndTargetWithRoll(position, target, roll);
    }
  }
}

void CGCamera::SetTargetFadeValue(BYTE value) {
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  FATALASSERT(worldFrame);
  worldFrame->SetPlayerFadeCameraValue(value);
}

void CGCamera::ToggleFreeLook() {
  if (m_flags & 0x8) {
    DisableFreeLook(0);
  } else {
    EnableFreeLook();
  }
}

void CGCamera::EnableFreeLook() {
  SetModeFreeLook();
}

void CGCamera::SetModeFreeLook() {
  if (!(m_flags & 0x8)) {
    m_flags |= 0x8;
    CGGameUI::HideCursor();
    EventSetMouseMode(MOUSE_MODE_RELATIVE, 0);
    CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
    FATALASSERT(worldFrame);
    worldFrame->OnMouseModeRelative();
    SysMsgAdd("Camera FREELOOK", SYSMSG_INFO, 4);
    m_previousPitch = m_pitch;
    if (!(m_flags & 0x7)) {
      CGObject_C *target = ClntObjMgrObjectPtr(m_target, __FILE__, __LINE__);
      FATALASSERT(target);
      m_yaw = target->GetFacing();
    }
    m_yawFreelookStart = m_yaw;
  }
}

void CGCamera::DisableFreeLook(int sticky) {
  SetModeNormal();
  m_distance = m_desiredDistance;

  UINT view = m_flags & 0x7;
  if (!view) {
    SetTargetFadeValue(0);
    return;
  }

  if (sticky) {
    m_desiredPitch = m_pitch;
    m_desiredYaw += m_yaw - m_yawFreelookStart;
    m_yawOffset = m_desiredYaw;
    return;
  }

  float       desiredAngle = m_views[view].yaw;
  CGObject_C *target = ClntObjMgrObjectPtr(m_target, __FILE__, __LINE__);
  if (target) {
    m_yawOffset = m_yaw - (target->IsA(ID_UNIT) ? static_cast<CGUnit_C *>(target)->GetRawSmoothFacing() : target->GetFacing());
    if (m_yawOffset < 0.0f) {
      m_yawOffset += 6.2831855f;
    } else if (m_yawOffset > 6.2831855f) {
      m_yawOffset -= 6.2831855f;
    }
  }

  if (NTempest::CMath::fabs_(desiredAngle - m_yawOffset) > PI) {
    desiredAngle = 6.2831855f - desiredAngle;
  }

  float motionTime = NTempest::CMath::fabs_(m_yawOffset - desiredAngle) / PI * s_cameraSmoothingTime->GetFloat();
  SetDesiredYawAngleOverTime(desiredAngle, motionTime, OsGetAsyncTimeMs());
  m_desiredPitch = m_pitch;
}

void CGCamera::SetModeNormal() {
  if (m_flags & 0x8) {
    m_flags &= ~0x8;
    CGGameUI::ShowCursor();
    EventSetMouseMode(MOUSE_MODE_NORMAL, 0);
    CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
    FATALASSERT(worldFrame);
    worldFrame->OnMouseModeNormal();
    SysMsgAdd("Camera NORMAL", SYSMSG_INFO, 4);
  }
}

void CGCamera::CreateViewFromParams(int view, float dist, float pitch, float yaw) {
  FATALASSERT((view > 0) && (view < MAX_CAMERA_VIEWS));
  m_views[view].dist = dist;
  m_views[view].pitch = pitch;
  m_views[view].yaw = yaw;
}

void CGCamera::CreateViewFromCamera(int view) {
  CreateViewFromParams(view, m_desiredDistance, m_desiredPitch - m_smoothingAngle, m_desiredYaw);
}

void CGCamera::NextView() {
  int view = (m_flags & 0x7) + 1;
  if (view < 5) {
    SetView(view);
  }
}

void CGCamera::PreviousView() {
  int view = m_flags & 0x7;
  if (view > 0) {
    SetView(view - 1);
  }
}

void CGCamera::ResetView(int view) {
  if (view < 0) {
    for (int i = 0; i < 5; ++i) {
      ResetView(i);
    }
    return;
  }

  FATALASSERT(view < MAX_CAMERA_VIEWS);
  switch (view) {
    case 0:
      m_views[view].dist = 0.0f;
      m_views[view].pitch = 0.0f;
      break;
    case 1:
      m_views[view].dist = s_cameraDistanceA->GetFloat();
      m_views[view].pitch = s_cameraAngleA->GetFloat() * 0.017453292f;
      break;
    case 2:
      m_views[view].dist = s_cameraDistanceB->GetFloat();
      m_views[view].pitch = s_cameraAngleB->GetFloat() * 0.017453292f;
      break;
    case 3:
      m_views[view].dist = s_cameraDistanceC->GetFloat();
      m_views[view].pitch = s_cameraAngleC->GetFloat() * 0.017453292f;
      break;
    case 4:
      m_views[view].dist = s_cameraDistanceD->GetFloat();
      m_views[view].pitch = s_cameraAngleD->GetFloat() * 0.017453292f;
      break;
    default:
      FATALERROR(("There is no default setting for this view"));
      break;
  }
  m_views[view].yaw = 0.0f;
  if (view == (m_flags & 0x7)) {
    SetView(m_flags & 0x7);
  }
}

void CGCamera::ZoomIn(float distance, DWORD timestamp) {
  DWORD timeout = static_cast<DWORD>(distance / s_cameraLinearSpeed->GetFloat() * 1000.0f);
  if (m_motionMask & (1 << (2 * CAMERA_MOVE_OUT))) {
    StopMotion(CAMERA_MOVE_OUT, timestamp);
  }
  if (m_motionTimeout[CAMERA_MOVE_IN] && (m_motionMask & (1 << (2 * CAMERA_MOVE_IN)))) {
    m_motionTimeout[CAMERA_MOVE_IN] += timeout;
  } else {
    StartMotion(CAMERA_MOVE_IN, timestamp, timeout);
  }
}

void CGCamera::ZoomOut(float distance, DWORD timestamp) {
  DWORD timeout = static_cast<DWORD>(distance / s_cameraLinearSpeed->GetFloat() * 1000.0f);
  if (m_motionMask & (1 << (2 * CAMERA_MOVE_IN))) {
    StopMotion(CAMERA_MOVE_IN, timestamp);
  }
  if (m_motionTimeout[CAMERA_MOVE_OUT] && (m_motionMask & (1 << (2 * CAMERA_MOVE_OUT)))) {
    m_motionTimeout[CAMERA_MOVE_OUT] += timeout;
  } else {
    StartMotion(CAMERA_MOVE_OUT, timestamp, timeout);
  }
}

void CGCamera::StartMotion(CGCameraMotion move, DWORD timestamp, DWORD timeout) {
  FATALASSERT(move < NUM_CAMERA_MOTIONS);
  if (m_flags & 7) {
    m_motionMask |= 1 << (2 * move);
    m_motionStart[move] = timestamp;
    if (timeout) {
      m_motionTimeout[move] = timestamp + timeout;
    } else {
      m_motionTimeout[move] = 0;
    }
  }
}

void CGCamera::StopMotion(CGCameraMotion move, DWORD timestamp) {
  FATALASSERT(move < NUM_CAMERA_MOTIONS);
  DWORD motion = 1 << (2 * move);
  if (m_motionMask & motion) {
    m_motionStop[move] = timestamp;
    m_motionMask |= 1 << (2 * move + 1);
  }
}

void CGCamera::UpdateFreeLookFacing(float dx, float dy) {
  DDCToNDC(dx, dy, &dx, &dy);
  dx = (dx * 0.00125f) * FREE_LOOK_SPEED;
  dy = (dy * 0.0016666667f) * FREE_LOOK_SPEED;
  m_pitch += (s_mouseInvertPitch->GetInt() ? -1.0f : 1.0f) * dy;
  if ((m_flags & 0x7) && s_mouseInvertYaw->GetInt()) {
    dx = -dx;
  }
  m_yaw -= dx;
  ClampAngles();

  float playerYaw = m_yaw - m_yawOffset;
  if (playerYaw < 0.0f) {
    playerYaw += 6.2831855f;
  }
  CGInputControl::GetActive()->CameraTurnPlayer(OsGetAsyncTimeMs(), playerYaw, m_pitch, false);
}

void CGCamera::SyncFreeLookFacing() {
  float playerYaw = m_yaw - m_yawOffset;
  if (playerYaw < 0.0f) {
    playerYaw += 6.2831855f;
  }

  CGInputControl::GetActive()->CameraTurnPlayer(OsGetAsyncTimeMs(), playerYaw, m_pitch, 1);
}

void CGCamera::UpdateMotion(DWORD timestamp) {
  for (int move = 0; move < NUM_CAMERA_MOTIONS; ++move) {
    if (!(m_motionMask & (1 << (2 * move)))) {
      continue;
    }

    if (m_motionTimeout[move] && static_cast<long>(timestamp - m_motionTimeout[move]) >= 0) {
      StopMotion(static_cast<CGCameraMotion>(move), timestamp);
    }

    DWORD elapsed;
    if (m_motionMask & (1 << (2 * move + 1))) {
      long stopped = m_motionStop[move] - m_motionStart[move];
      elapsed = stopped < 0 ? 0 : stopped;
      m_motionMask &= ~(3 << (2 * move));
    } else {
      long running = timestamp - m_motionStart[move];
      if (running < 0) {
        continue;
      }
      elapsed = running;
      m_motionStart[move] = timestamp;
    }

    switch (move) {
      case CAMERA_MOVE_IN:
        if (!m_zoomSmoothingTimestamp) {
          m_desiredDistance -= s_cameraLinearSpeed->GetFloat() * elapsed * 0.001f;
          if (m_desiredDistance < MIN_CAMERA_DIST + m_nearZ) {
            m_desiredDistance = MIN_CAMERA_DIST + m_nearZ;
          }
          m_distance = m_desiredDistance;
        }
        break;

      case CAMERA_MOVE_OUT:
        if (!m_zoomSmoothingTimestamp) {
          m_desiredDistance += s_cameraLinearSpeed->GetFloat() * elapsed * 0.001f;
          if (m_desiredDistance > MAX_CAMERA_DIST) {
            m_desiredDistance = MAX_CAMERA_DIST;
          }
          m_distance = m_desiredDistance;
        }
        break;

      case CAMERA_MOVE_RIGHT:
        if (!m_yawSmoothingTimestamp) {
          m_desiredYaw += (s_cameraAngularSpeed->GetFloat() * elapsed * 0.001f) * 0.017453292f;
          if (m_desiredYaw > 6.2831855f) {
            m_desiredYaw -= 6.2831855f;
          }
          m_yawOffset = m_desiredYaw;
        }
        break;

      case CAMERA_MOVE_LEFT:
        if (!m_yawSmoothingTimestamp) {
          m_desiredYaw -= (s_cameraAngularSpeed->GetFloat() * elapsed * 0.001f) * 0.017453292f;
          if (m_desiredYaw < 0.0f) {
            m_desiredYaw += 6.2831855f;
          }
          m_yawOffset = m_desiredYaw;
        }
        break;

      case CAMERA_MOVE_UP:
        if (!m_pitchSmoothingTimestamp) {
          m_desiredPitch += (s_cameraAngularSpeed->GetFloat() * elapsed * 0.00025f) * 0.017453292f;
          if (m_desiredPitch - m_smoothingAngle > MAX_PITCH_ANGLE) {
            m_desiredPitch = MAX_PITCH_ANGLE + m_smoothingAngle;
          }
          m_pitch = m_desiredPitch;
        }
        break;

      case CAMERA_MOVE_DOWN:
        if (!m_pitchSmoothingTimestamp) {
          m_desiredPitch -= (s_cameraAngularSpeed->GetFloat() * elapsed * 0.00025f) * 0.017453292f;
          float minimum = !(m_flags & 7) ? -MAX_PITCH_ANGLE : MIN_PITCH_ANGLE;
          if (m_desiredPitch - m_smoothingAngle < minimum) {
            m_desiredPitch = minimum + m_smoothingAngle;
          }
          m_pitch = m_desiredPitch;
        }
        break;
    }
  }
}

void CGCamera::RunShakes() {
  if (!m_shakes.IsEmpty()) {
    NTempest::C3Vector shakeOffset(0.0f);
    DWORD              timestamp = OsGetAsyncTimeMs();
    CGUnit_C          *target = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(m_target, __FILE__, __LINE__));
    FATALASSERT(target);
    float yaw = target->GetSmoothFacing();

    ITERATELIST(CameraShake, m_shakes, shake) {
      float time = (timestamp - shake->m_timestamp) * 0.001f + shake->m_phase;
      if (time >= shake->m_duration) {
        ITERATE_DELETE
      }

      float amount = static_cast<float>(sin(time * shake->m_frequency * 6.2831855f)) * shake->m_amplitude;
      if (shake->m_shakeType == 1) {
        amount *= static_cast<float>(exp(-time * shake->m_coefficient));
      }

      if (shake->m_direction == 0) {
        shakeOffset.x += static_cast<float>(cos(yaw)) * amount;
        shakeOffset.y += static_cast<float>(sin(yaw)) * amount;
      } else if (shake->m_direction == 1) {
        float right = yaw + 1.5707964f;
        shakeOffset.x += static_cast<float>(cos(right)) * amount;
        shakeOffset.y += static_cast<float>(sin(right)) * amount;
      } else if (shake->m_direction == 2) {
        shakeOffset.z += amount;
      }
    }

    m_position += shakeOffset;
  }
}

void CGCamera::CheckUnderwater() {
  UINT liquid = CWorld::SceneCamLiquidStatus();
  if (!(m_flags & 0x20) || m_savedLiquid != liquid) {
    SndInterfaceSetUnderwater(liquid != 15);
    m_flags |= 0x20;
    m_savedLiquid = liquid;
  }
}

BOOL CGCamera::UpdateCallback(LPCVOID, LPVOID param) {
  if (param) {
    CGCamera *camera = static_cast<CGCamera *>(param);
    DWORD     timestamp = OsGetAsyncTimeMs();
    camera->m_fov = s_cameraFOV->GetFloat() * 0.017453292f;
    camera->m_nearZ = s_cameraNearZ->GetFloat();
    camera->m_farZ = s_cameraFarZ->GetFloat();

    if (camera->m_model) {
      camera->CalcModelCamera(timestamp);
      camera->CheckUnderwater();
      return 1;
    }

    CGObject_C *target = ClntObjMgrObjectPtr(camera->m_target, __FILE__, __LINE__);
    if (target) {
      if (camera->m_flags & 7) {
        camera->CalcThirdPerson(target, timestamp);
      } else {
        if (camera->m_distance > 0.027777778f) {
          camera->CalcThirdPerson(target, timestamp);
          if (camera->m_distance > 0.027777778f) {
            camera->RunShakes();
            camera->CheckUnderwater();
            return 1;
          }
          camera->SetTargetFadeValue(0);
          camera->m_distance = 0.0f;
        }
        camera->CalcFirstPerson(target, timestamp);
      }
      camera->RunShakes();
      camera->CheckUnderwater();
    }
  }
  return 1;
}

void CGCamera::SetupWorldProjection(const NTempest::CRect &projectionRect) {
  SetGxProjectionAndView(projectionRect);
}

void CGCamera::MakeRelativeTo(DWORDLONG guid) {
  if (guid == m_relativeTo) {
    return;
  }

  if (m_relativeTo) {
    CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(m_relativeTo, __FILE__, __LINE__));
    FATALASSERT(transport);
    FATALASSERT(transport->IsA(TYPE_GAMEOBJECT));
    FATALASSERT(transport->IsTransport());

    m_yaw += transport->GetFacing();
    if (m_yaw > 6.2831855f) {
      m_yaw -= 6.2831855f;
    }
  }

  m_relativeTo = guid;

  if (guid) {
    CGGameObject_C *transport = static_cast<CGGameObject_C *>(ClntObjMgrObjectPtr(guid, __FILE__, __LINE__));
    FATALASSERT(transport);
    FATALASSERT(transport->IsA(TYPE_GAMEOBJECT));
    FATALASSERT(transport->IsTransport());

    m_yaw -= transport->GetFacing();
    if (m_yaw < 0.0f) {
      m_yaw += 6.2831855f;
    }
  }

  CSimpleCamera::SetFacing(m_yaw, m_pitch, m_roll);
}

NTempest::C33Matrix CGCamera::ParentToWorld() const {
  FATALASSERT(m_relativeTo);

  NTempest::C3Vector  zAxis(0.0f, 0.0f, 1.0f);
  NTempest::C33Matrix rotation;
  CGObject_C         *transport = ClntObjMgrObjectPtr(m_relativeTo, __FILE__, __LINE__);
  FATALASSERT(transport);
  rotation.Rotate(transport->GetFacing(), zAxis, 1);
  return rotation;
}

NTempest::C3Vector CGCamera::Forward() const {
  if (!m_relativeTo) {
    return CSimpleCamera::Forward();
  }
  return CSimpleCamera::Forward() * ParentToWorld();
}

NTempest::C3Vector CGCamera::Right() const {
  if (!m_relativeTo) {
    return CSimpleCamera::Right();
  }
  return CSimpleCamera::Right() * ParentToWorld();
}

NTempest::C3Vector CGCamera::Up() const {
  if (!m_relativeTo) {
    return CSimpleCamera::Up();
  }
  return CSimpleCamera::Up() * ParentToWorld();
}

static char *modeText[5] = {"FIRST_PERSON", "THIRD_PERSON_A", "THIRD_PERSON_B", "THIRD_PERSON_C", "THIRD_PERSON_D"};

void CGCamera::SetView(int newView) {
  FATALASSERT(newView < MAX_CAMERA_VIEWS);

  if ((m_flags & 0x7) == newView) {
    m_zoomSmoothingTimestamp = 0;
    if (!(m_flags & 0x10)) {
      m_pitchSmoothingTimestamp = 0;
    }
    m_yawSmoothingTimestamp = 0;
    return;
  }

  SysMsgPrintf(SYSMSG_INFO, 4, "Camera view %s", modeText[newView]);
  DWORD timestamp = OsGetAsyncTimeMs();
  m_flags = (m_flags & ~0x7) | newView;
  SetDesiredDistance(m_views[newView].dist, timestamp);
  SetDesiredPitchAngle(m_views[newView].pitch, 0.0f, timestamp);
  SetDesiredYawAngle(m_views[newView].yaw, 0.0f, timestamp);
}

void CGCamera::CycleView() {
  int view = (m_flags & 0x7) + m_cycleDirection;
  if (view >= MAX_CAMERA_VIEWS) {
    view = 3;
    m_cycleDirection = -m_cycleDirection;
  } else if (view < 0) {
    m_cycleDirection = -m_cycleDirection;
    view = 1;
  }
  SetView(view);
}

void CGCamera::ResetModelCamera() {
  if (m_model) {
    ModelSetSequence(m_model, 0, 8u);
  }
}

BOOL CGCamera::SetModelCamera(LPCSTR modelFile, const NTempest::C3Vector &origin, float facing, int (*ModelCameraFinished)(LPVOID), LPVOID param) {
  ClearModelCamera();

  CStatus status;
  m_model = ModelCreate(modelFile, 0, &status);
  SysMsgAdd(status, 0x10);
  if (!m_model) {
    return 0;
  }

  ModelSetSeqFinishedHandler(m_model, 0, ModelCameraFinished, param);
  m_modelMatrix.Identity();
  m_modelMatrix.Translate(origin);
  m_modelMatrix.Rotate(facing, NTempest::C3Vector(0.0f, 0.0f, 1.0f), 1);

  while (!ModelIsLoaded(m_model, 1)) {
    AsyncFileReadWaitAll();
  }

  if (!ModelIsLoaded(m_model, 1)) {
    m_flags &= ~0x40u;
  } else if (!FinishLoadingModel()) {
    return 0;
  }

  ResetModelCamera();
  return 1;
}

void CGCamera::SetPositionAndTarget(const NTempest::C3Vector &position, const NTempest::C3Vector &target) {
  NTempest::C3Vector facing(target.x - position.x, target.y - position.y, target.z - position.z);
  float              magnitude = facing.Mag();
  if (NTempest::CMath::fnotequal_(magnitude, 0.0f)) {
    facing *= 1.0f / magnitude;
  }

  m_position = position;
  CSimpleCamera::SetFacing(facing);
}

void CGCamera::SetPositionAndFacing(const NTempest::C3Vector &position, const NTempest::C3Vector &facing) {
  m_position = position;
  CSimpleCamera::SetFacing(facing);
}

float CGCamera::GetSmoothedHeight(float z, int moving) {
  if (m_savedTargetZ == 0.0f) {
    m_savedTargetZ = z;
    return z;
  }

  if (!moving) {
    return m_savedTargetZ;
  }

  float delta = z - m_savedTargetZ;
  if (NTempest::CMath::fabs_(delta) < m_maxSmoothZ) {
    delta /= m_maxSmoothZ;
    delta *= NTempest::CMath::fabs_(delta);
    delta *= m_maxSmoothZ;
  }
  m_savedTargetZ += delta;
  return m_savedTargetZ;
}

void CGCamera::SetDesiredDistance(float desiredDistance, DWORD timestamp) {
  if (NTempest::CMath::fnotequal_(m_desiredDistance, desiredDistance)) {
    m_zoomTime = NTempest::CMath::fabs_((desiredDistance - m_distance) / (desiredDistance - m_desiredDistance)) * s_cameraSmoothingTime->GetFloat();
    m_zoomSmoothingTimestamp = timestamp;
    m_previousDistance = m_distance;
    m_desiredDistance = desiredDistance;
  }
}

void CGCamera::SetDesiredDistanceOverTime(float desiredDistance, float motionTime, DWORD timestamp) {
  m_zoomTime = motionTime;
  m_zoomSmoothingTimestamp = timestamp;
  m_previousDistance = m_distance;
  m_desiredDistance = desiredDistance;
}

void CGCamera::SetDesiredPitchAngle(float desiredAngle, float delay, DWORD timestamp) {
  if (NTempest::CMath::fnotequal_(m_desiredPitch, desiredAngle)) {
    m_flags &= ~0x10u;
    m_smoothingAngle = 0.0f;
    float motionTime = NTempest::CMath::fabs_((desiredAngle - m_pitch) / (desiredAngle - m_desiredPitch)) * s_cameraSmoothingTime->GetFloat();
    SetDesiredPitchAngleOverTime(desiredAngle, motionTime, timestamp + static_cast<DWORD>(delay * 1000.0f));
  }
}

void CGCamera::SetDesiredPitchAngleOverTime(float desiredAngle, float motionTime, DWORD timestamp) {
  m_previousPitch = m_pitch;
  m_desiredPitch = desiredAngle;
  m_pitchTime = motionTime;
  m_pitchSmoothingTimestamp = timestamp;
}

void CGCamera::SetDesiredYawAngle(float desiredAngle, float delay, DWORD timestamp) {
  if (NTempest::CMath::fabs_(desiredAngle - m_yawOffset) > PI) {
    desiredAngle = 6.2831855f - desiredAngle;
  }
  if (m_desiredYaw != desiredAngle) {
    float motionTime = (desiredAngle - m_yawOffset) / (desiredAngle - m_desiredYaw) * s_cameraSmoothingTime->GetFloat();
    SetDesiredYawAngleOverTime(desiredAngle, motionTime, timestamp + static_cast<DWORD>(delay * 1000.0f));
  }
}

void CGCamera::SetDesiredYawAngleOverTime(float desiredAngle, float motionTime, DWORD timestamp) {
  m_previousYaw = m_yawOffset;
  m_desiredYaw = desiredAngle;
  m_yawTime = motionTime;
  m_yawSmoothingTimestamp = timestamp;
}

void CGCamera::SetSmoothingAngle(float smoothingAngle, DWORD timestamp, int quickly) {
  if (smoothingAngle > CAMERA_SMOOTH_MAXIMUM_ANGLE) {
    smoothingAngle = CAMERA_SMOOTH_MAXIMUM_ANGLE;
  } else if (smoothingAngle < CAMERA_SMOOTH_MINIMUM_ANGLE) {
    smoothingAngle = CAMERA_SMOOTH_MINIMUM_ANGLE;
  }

  float newAngle = m_desiredPitch - m_smoothingAngle + smoothingAngle;
  if (newAngle < -1.5707964f) {
    newAngle = -1.5707964f;
  } else if (newAngle > 1.5707964f) {
    newAngle = 1.5707964f;
  }

  if (NTempest::CMath::fnotequal_(newAngle, m_desiredPitch)) {
    float smoothTime = CAMERA_SMOOTH_TIME;
    if (quickly) {
      smoothTime = CAMERA_SMOOTH_TIME * 0.25f;
    }
    SetDesiredPitchAngleOverTime(newAngle, smoothTime, timestamp);
  }
  m_smoothingAngle = smoothingAngle;
}

void CGCamera::PerformTerrainTilt(DWORD timestamp, NTempest::C3Vector position, float facing, int moving, int turning, int updateOnly) {
  int quickly;
  static const struct {
    float slope;
    float angle;
  } tanTable[10] = {
      {0.00f, 0.00000000f},
      {0.09f, 0.08726646f},
      {0.18f, 0.17453292f},
      {0.27f, 0.26179939f},
      {0.36f, 0.34906584f},
      {0.47f, 0.43633231f},
      {0.58f, 0.52359879f},
      {0.70f, 0.61086523f},
      {0.84f, 0.69813168f},
      {1.00f, 0.78539818f},
  };

  if (!s_cameraSmooth->GetInt()) {
    return;
  }
  if ((m_flags & 0x8) || (m_flags & 0x7) != 1) {
    return;
  }

  DWORD nextUpdate;
  if (moving || !turning) {
    quickly = 0;
    nextUpdate = m_lastDeltaZ + DELTA_Z_FREQUENCY;
  } else {
    quickly = 1;
    nextUpdate = m_lastDeltaZ + 25;
  }

  if (!updateOnly && static_cast<long>(timestamp - nextUpdate) < 0) {
    return;
  }
  m_lastDeltaZ = timestamp;

  NTempest::C3Vector pos[5];
  position.z += WAIST_HEIGHT;
  pos[0] = position;

  NTempest::C3Vector direction(NTempest::CMath::cos_(facing), NTempest::CMath::sin_(facing), 0.0f);
  pos[1] = position + direction * FOCAL_DISTANCE;
  pos[2] = pos[1];

  float dist = 1.0f;
  CWorld::Intersect(&pos[0], &pos[1], 0.0f, &pos[2], &dist, 0x111);

  pos[2] = NTempest::C3Vector(pos[2].x - direction.x * 0.27777779f, pos[2].y - direction.y * 0.27777779f, pos[2].z);
  pos[3] = NTempest::C3Vector(pos[2].x, pos[2].y, pos[2].z - 7.1111112f);
  pos[4] = pos[3];
  dist = 1.0f;
  CWorld::Intersect(&pos[2], &pos[3], 0.0f, &pos[4], &dist, 0x111);

  float slope = (pos[4].z - pos[2].z + WAIST_HEIGHT)
              / NTempest::CMath::sqrt_((pos[2].y - pos[0].y) * (pos[2].y - pos[0].y) + (pos[2].x - pos[0].x) * (pos[2].x - pos[0].x));

  float sign;
  if (slope < 0.0f) {
    slope = -slope;
    sign = 1.0f;
  } else {
    sign = -1.0f;
  }

  float smoothingAngle = 0.0f;
  int   index = 10;
  do {
    --index;
    if (slope >= tanTable[index].slope) {
      float angle = tanTable[index].angle;
      if (angle > minSmoothAngle) {
        smoothingAngle = (angle - minSmoothAngle) * sign;
      } else {
        smoothingAngle = 0.0f;
      }
      break;
    }
  } while (index);
  if (updateOnly) {
    m_smoothingAngle = smoothingAngle;
  } else {
    SetSmoothingAngle(smoothingAngle, timestamp, quickly);
  }
}
