#include "InputControl.h"

#include <Console/ConsoleCommand.h>
#include <Console/ConsoleVar.h>
#include <Event/CMouseEvent.h>
#include <FrameScript/FrameScript.h>
#include <Os/OsTime.h>
#include <Os/W32/OsJoystick.h>
#include <Gx/Gx.h>

#include "Object/ObjectClient/Player_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "UIUtil/Camera.h"
#include "Ui/WorldFrame.h"

#include <storm.h>

#include <stdlib.h>
#include <math.h>

#include <lua.h>

static int          s_joystickID = -1;
static unsigned int s_buttonState;
static float        s_speed;
static float        s_delta[2];
static float        s_rate;
static const float  DELTAX_PER_SECOND = 512.0f;
static const float  DELTAY_PER_SECOND = 192.0f;
static int          AXIS_THRESHOLD = 0x1FFF;

CGInputControl *CGInputControl::s_inputControl;

static bool JoystickCallback(CVar *cvar, const char *oldValue, const char *newValue, void *userArg) {
  if (*newValue == '1') {
    if (s_joystickID == -1) {
      s_joystickID = OsOpenJoystick(0);
    }
  } else if (s_joystickID != -1) {
    OsCloseJoystick(s_joystickID);
    s_joystickID = -1;
  }

  return true;
}

static int Script_ToggleAutoRun(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);

  unsigned long eventTime = lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs();
  control->SetControlBit(INPUT_MOVE_PLAYER_AUTORUN, !(control->m_controlFlags & INPUT_MOVE_PLAYER_AUTORUN), eventTime, 0);
  return 0;
}

static int Script_MoveForwardStart(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_MOVE_PLAYER_FORWARD_KEY, 1, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_MoveForwardStop(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_MOVE_PLAYER_FORWARD_KEY, 0, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_MoveBackwardStart(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_MOVE_PLAYER_BACKWARD_KEY, 1, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_MoveBackwardStop(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_MOVE_PLAYER_BACKWARD_KEY, 0, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_TurnLeftStart(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_TURN_PLAYER_LEFT_KEY, 1, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_TurnLeftStop(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_TURN_PLAYER_LEFT_KEY, 0, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_TurnRightStart(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_TURN_PLAYER_RIGHT_KEY, 1, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_TurnRightStop(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_TURN_PLAYER_RIGHT_KEY, 0, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_StrafeLeftStart(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_STRAFE_PLAYER_LEFT_KEY, 1, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_StrafeLeftStop(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_STRAFE_PLAYER_LEFT_KEY, 0, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_StrafeRightStart(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_STRAFE_PLAYER_RIGHT_KEY, 1, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_StrafeRightStop(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_STRAFE_PLAYER_RIGHT_KEY, 0, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_PitchUpStart(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_PITCH_PLAYER_UP_KEY, 1, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_PitchUpStop(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_PITCH_PLAYER_UP_KEY, 0, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_PitchDownStart(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_PITCH_PLAYER_DOWN_KEY, 1, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_PitchDownStop(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_PITCH_PLAYER_DOWN_KEY, 0, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_TurnOrActionStart(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  unsigned long eventTime = lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs();
  control->SetReleaseAction(INPUT_RELEASE_ACTION);
  control->SetControlBit(INPUT_TURN_PLAYER, 1, eventTime, 0);
  return 0;
}

static int Script_TurnOrActionStop(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  control->SetControlBit(INPUT_TURN_PLAYER, 0, lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs(), 0);
  return 0;
}

static int Script_CameraOrSelectOrMoveStart(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  unsigned long eventTime = lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs();
  control->SetReleaseAction(INPUT_RELEASE_SELECT);
  control->SetControlBit(INPUT_MOVE_PLAYER_OR_TURN_CAMERA, 1, eventTime, 0);
  return 0;
}

static int Script_CameraOrSelectOrMoveStop(lua_State *L) {
  CGInputControl *control = CGInputControl::GetActive();
  FATALASSERT(control);
  unsigned long eventTime = lua_isnumber(L, 1) ? static_cast<unsigned long>(lua_tonumber(L, 1)) : OsGetAsyncTimeMs();
  int           updatePlayer = 0;
  if (lua_isnumber(L, 2)) {
    updatePlayer = static_cast<int>(lua_tonumber(L, 2));
  } else if (lua_isstring(L, 2)) {
    updatePlayer = atoi(lua_tostring(L, 2));
  }
  control->SetControlBit(INPUT_MOVE_PLAYER_OR_TURN_CAMERA, 0, eventTime, updatePlayer);
  return 0;
}

static const FrameScript_Method s_ScriptFunctions[21] = {
    {            "ToggleAutoRun",             Script_ToggleAutoRun},
    {         "MoveForwardStart",          Script_MoveForwardStart},
    {          "MoveForwardStop",           Script_MoveForwardStop},
    {        "MoveBackwardStart",         Script_MoveBackwardStart},
    {         "MoveBackwardStop",          Script_MoveBackwardStop},
    {            "TurnLeftStart",             Script_TurnLeftStart},
    {             "TurnLeftStop",              Script_TurnLeftStop},
    {           "TurnRightStart",            Script_TurnRightStart},
    {            "TurnRightStop",             Script_TurnRightStop},
    {          "StrafeLeftStart",           Script_StrafeLeftStart},
    {           "StrafeLeftStop",            Script_StrafeLeftStop},
    {         "StrafeRightStart",          Script_StrafeRightStart},
    {          "StrafeRightStop",           Script_StrafeRightStop},
    {             "PitchUpStart",              Script_PitchUpStart},
    {              "PitchUpStop",               Script_PitchUpStop},
    {           "PitchDownStart",            Script_PitchDownStart},
    {            "PitchDownStop",             Script_PitchDownStop},
    {        "TurnOrActionStart",         Script_TurnOrActionStart},
    {         "TurnOrActionStop",          Script_TurnOrActionStop},
    {"CameraOrSelectOrMoveStart", Script_CameraOrSelectOrMoveStart},
    { "CameraOrSelectOrMoveStop",  Script_CameraOrSelectOrMoveStop}
};

static CGCamera *Camera() {
  CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
  FATALASSERT(worldFrame);
  return CGWorldFrame::GetActiveCamera();
}

void InputControlInitialize() {
  CVar::Register("Joystick", "enable joystick control", 0, "0", JoystickCallback, DEFAULT, false, 0);

  CGInputControl::Initialize();
}

void InputControlRegisterScriptFunctions() {
  for (int i = 0; i < 21; ++i) {
    FrameScript_RegisterFunction(s_ScriptFunctions[i].name, s_ScriptFunctions[i].method);
  }
}

void InputControlUnregisterScriptFunctions() {
  for (int i = 0; i < 21; ++i) {
    FrameScript_UnregisterFunction(s_ScriptFunctions[i].name);
  }
}

void InputControlDestroy() {
  CGInputControl::Destroy();
}

CGInputControl::CGInputControl() {
  m_controlFlags = 0;
  m_mouseDownTime = 0;
  m_releaseAction = INPUT_RELEASE_NONE;
  m_initializeTime = OsGetAsyncTimeMs();
}

CGInputControl *CGInputControl::GetActive() {
  ASSERT(s_inputControl);
  return s_inputControl;
}

void CGInputControl::OnUpdate(float elapsedSec) {
  struct {
    int   absvalue;
    float speed[2];
  } static deltas[16] = {
      {0x0000,                                                           {0.0f, 0.0f}},
      {0x07FF,                                                           {0.0f, 0.0f}},
      {0x0FFF,                                                           {0.0f, 0.0f}},
      {0x17FF,                                                           {0.0f, 0.0f}},
      {0x1FFF,                 {DELTAX_PER_SECOND / 12.0f, DELTAY_PER_SECOND / 12.0f}},
      {0x27FF,                   {DELTAX_PER_SECOND / 6.0f, DELTAY_PER_SECOND / 6.0f}},
      {0x2FFF,                   {DELTAX_PER_SECOND / 4.0f, DELTAY_PER_SECOND / 4.0f}},
      {0x37FF,                   {DELTAX_PER_SECOND / 3.0f, DELTAY_PER_SECOND / 3.0f}},
      {0x3FFF,   {DELTAX_PER_SECOND * 5.0f / 12.0f, DELTAY_PER_SECOND * 5.0f / 12.0f}},
      {0x47FF,                   {DELTAX_PER_SECOND / 2.0f, DELTAY_PER_SECOND / 2.0f}},
      {0x4FFF,   {DELTAX_PER_SECOND * 7.0f / 12.0f, DELTAY_PER_SECOND * 7.0f / 12.0f}},
      {0x57FF,     {DELTAX_PER_SECOND * 2.0f / 3.0f, DELTAY_PER_SECOND * 2.0f / 3.0f}},
      {0x5FFF,     {DELTAX_PER_SECOND * 3.0f / 4.0f, DELTAY_PER_SECOND * 3.0f / 4.0f}},
      {0x67FF,     {DELTAX_PER_SECOND * 5.0f / 6.0f, DELTAY_PER_SECOND * 5.0f / 6.0f}},
      {0x6FFF, {DELTAX_PER_SECOND * 11.0f / 12.0f, DELTAY_PER_SECOND * 11.0f / 12.0f}},
      {0x77FF,                                 {DELTAX_PER_SECOND, DELTAY_PER_SECOND}}
  };

  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player || s_joystickID == -1) {
    return;
  }

  unsigned long eventTime = OsGetAsyncTimeMs();
  int           value = OsGetButtonState(s_joystickID);
  if (static_cast<unsigned int>(value) != s_buttonState) {
    if ((value ^ s_buttonState) & 0x1) {
      SetControlBit(INPUT_TURN_PLAYER, value & 0x1, eventTime, 0);
    }
    if ((value ^ s_buttonState) & 0x2) {
      SetControlBit(INPUT_MOVE_PLAYER_FORWARD_KEY, value & 0x2, eventTime, 0);
    }
    s_buttonState = value;
  }

  float delta;
  float deltaY;
  float rate;
  int   i;
  if (s_buttonState & 0x1) {
    value = OsGetAxisState(s_joystickID, 0);
    delta = 0.0f;
    for (i = 15; i >= 0; --i) {
      if (abs(value) >= deltas[i].absvalue) {
        delta = elapsedSec * deltas[i].speed[0];
        break;
      }
    }
    if (value < 0) {
      delta = -delta;
    }
    if (delta > s_delta[0]) {
      s_delta[0] += delta - s_delta[0] > 1.0f ? 1.0f : delta - s_delta[0];
    } else {
      s_delta[0] -= s_delta[0] - delta > 1.0f ? 1.0f : s_delta[0] - delta;
    }

    value = OsGetAxisState(s_joystickID, 1);
    deltaY = 0.0f;
    for (i = 15; i >= 0; --i) {
      if (abs(value) >= deltas[i].absvalue) {
        deltaY = elapsedSec * deltas[i].speed[1];
        break;
      }
    }
    if (value < 0) {
      deltaY = -deltaY;
    }
    if (deltaY > s_delta[1]) {
      s_delta[1] += deltaY - s_delta[1] > 1.0f ? 1.0f : deltaY - s_delta[1];
    } else {
      s_delta[1] -= s_delta[1] - deltaY > 1.0f ? 1.0f : s_delta[1] - deltaY;
    }
    Camera()->UpdateFreeLookFacing(s_delta[0], -s_delta[1]);
  } else {
    value = OsGetAxisState(s_joystickID, 0);
    if (abs(value) > AXIS_THRESHOLD) {
      rate = static_cast<float>(abs(value) - AXIS_THRESHOLD) / static_cast<float>(0x7FFF - AXIS_THRESHOLD) * 3.1415927f;
      if (fabs(s_rate - rate) >= 0.00000023841858f) {
        if (rate > s_rate) {
          s_rate += rate - s_rate > 3.1415927f * 0.1f ? 3.1415927f * 0.1f : rate - s_rate;
        } else {
          s_rate -= s_rate - rate > 3.1415927f * 0.1f ? 3.1415927f * 0.1f : s_rate - rate;
        }
        player->OnTurnRateChangeLocal(eventTime, rate);
      }
      if (value > 0) {
        SetControlBit(INPUT_TURN_PLAYER_LEFT_KEY, 0, eventTime, 0);
        SetControlBit(INPUT_TURN_PLAYER_RIGHT_KEY, 1, eventTime, 0);
      } else {
        SetControlBit(INPUT_TURN_PLAYER_LEFT_KEY, 1, eventTime, 0);
        SetControlBit(INPUT_TURN_PLAYER_RIGHT_KEY, 0, eventTime, 0);
      }
    } else if (fabs(s_rate) >= 0.00000023841858f) {
      s_rate = 0.0f;
      player->OnTurnRateChangeLocal(eventTime, 3.1415927f);
      SetControlBit(INPUT_TURN_PLAYER_LEFT_KEY, 0, eventTime, 0);
      SetControlBit(INPUT_TURN_PLAYER_RIGHT_KEY, 0, eventTime, 0);
    }
  }

  value = -OsGetAxisState(s_joystickID, 2);
  float speed;
  if (value > 0) {
    speed = static_cast<float>(value) * (1.0f / 32767.0f) * 30.0f;
    if (fabs(s_speed - speed) >= 0.00000023841858f) {
      if (speed > s_speed) {
        s_speed += speed - s_speed > 0.5f ? 0.5f : speed - s_speed;
      } else {
        s_speed -= s_speed - speed > 0.5f ? 0.5f : s_speed - speed;
      }
      player->OnAllSpeedChangeLocal(eventTime, speed);
    }
  } else if (fabs(s_speed) >= 0.00000023841858f) {
    s_speed = 0.0f;
    player->OnAllSpeedChangeLocal(eventTime, 7.5f);
  }
}

void CGInputControl::UpdatePlayer(unsigned long now) {
  CGUnit_C *player = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGUnit_C::m_activeMover, __FILE__, __LINE__));
  if (!player) {
    return;
  }

  if ((player->m_flags & 0x200) && static_cast<long>(now - player->m_animEndTime) < 0) {
    now = player->m_animEndTime;
  }

  const CGUnitData *unit = player->GetUnitData();
  bool              canIssueMovement = (unit->flags & 0x1000000) || ((player->GetType() & TYPE_PLAYER) && !unit->charm &&
                                                                     ((unit->flags & 2) || !(unit->flags & 0xC00004)) && !(unit->flags & 1));
  bool              canMove = unit->health > 0 && canIssueMovement && !player->IsInStandSitTransition() &&
                              !(player->m_move.m_moveFlags & 0x2400);
  bool              canTurn = !(unit->flags & 0x40000);

  if (canMove) {
    MovePlayer(now, player);
    StrafePlayer(now, player);
  } else {
    if (m_controlFlags & INPUT_MOVE_PLAYER_SENT) {
      if (canIssueMovement) {
        player->OnMoveStopLocal(now);
      }
      m_controlFlags &= ~INPUT_MOVE_PLAYER_SENT;
    }
    if (m_controlFlags & INPUT_STRAFE_PLAYER_SENT) {
      if (canIssueMovement) {
        player->OnStrafeStopLocal(now);
      }
      m_controlFlags &= ~INPUT_STRAFE_PLAYER_SENT;
    }
  }

  if (canTurn) {
    TurnPlayer(now, player);
    if (player->m_flags & 0x2000000) {
      PitchPlayer(now, player);
    }
  } else {
    if (m_controlFlags & INPUT_TURN_PLAYER_SENT) {
      if (canIssueMovement) {
        player->OnTurnStopLocal(now);
      }
      m_controlFlags &= ~INPUT_TURN_PLAYER_SENT;
    }
    if (m_controlFlags & INPUT_PITCH_PLAYER_SENT) {
      if (canIssueMovement) {
        player->OnPitchStopLocal(now);
      }
      m_controlFlags &= ~INPUT_PITCH_PLAYER_SENT;
    }
  }
}

void CGInputControl::SetReleaseAction(CGInputReleaseAction action) {
  if (!(m_controlFlags & INPUT_FREE_LOOK_MASK)) {
    m_releaseAction = action;
  }
}

int CGInputControl::SetControlBit(INPUT_CONTROL bit) {
  if (m_controlFlags & bit) {
    return 0;
  }

  if (!(m_controlFlags & INPUT_FREE_LOOK_MASK) && (bit & INPUT_FREE_LOOK_MASK)) {
    m_mouseChangeX = 0.0f;
    m_mouseChangeY = 0.0f;
    m_mouseDownTime = OsGetAsyncTimeMs();
    Camera()->EnableFreeLook();
  }

  m_controlFlags |= bit;
  if (bit & INPUT_MOVE_PLAYER_MASK) {
    m_controlFlags &= ~INPUT_MOVE_PLAYER_AUTORUN;
  }
  if ((m_controlFlags & INPUT_MOVE_AND_TURN_PLAYER) == INPUT_MOVE_AND_TURN_PLAYER) {
    m_controlFlags &= ~INPUT_MOVE_PLAYER_AUTORUN;
  }
  if (bit & INPUT_MOVE_PLAYER_MASK) {
    m_controlFlags |= INPUT_PLAYER_MOVED;
  }
  if (bit & INPUT_FREE_LOOK_MASK) {
    m_controlFlags |= INPUT_CAMERA_MOVED;
    if ((m_controlFlags & INPUT_FREE_LOOK_MASK) != static_cast<unsigned int>(bit)) {
      m_releaseAction = INPUT_RELEASE_NONE;
    }
  }
  return 1;
}

void CGInputControl::SetControlBit(INPUT_CONTROL bit, int set, unsigned long now, int sticky) {
  int changed = set ? SetControlBit(bit) : UnsetControlBit(bit, sticky);
  if (changed) {
    UpdatePlayer(now);
  }
}

int CGInputControl::IsMouseDragging() const {
  if (static_cast<int>(OsGetAsyncTimeMs() - m_mouseDownTime - 800) >= 0) {
    return 1;
  }

  if (m_mouseChangeX >= 8.0f || m_mouseChangeY >= 8.0f) {
    return static_cast<int>(OsGetAsyncTimeMs() - m_mouseDownTime - 200) >= 0;
  }

  return 0;
}

int CGInputControl::IsMouseDragMoving() const {
  if (!IsMouseDragging()) {
    return 0;
  }

  return GxPerfCounter(GxPerf_FrameNum) - m_lastFrameMouseMoved < 2;
}

void CGInputControl::OnMouseMoveRel(const CMouseEvent &evt) {
  if (m_controlFlags) {
    m_mouseChangeX += static_cast<float>(fabs(evt.x));
    m_mouseChangeY += static_cast<float>(fabs(evt.y));
    m_lastFrameMouseMoved = GxPerfCounter(GxPerf_FrameNum);
    Camera()->UpdateFreeLookFacing(evt.x, evt.y);
  }
}

int CGInputControl::UnsetControlBit(INPUT_CONTROL bit, int sticky) {
  if (!(m_controlFlags & bit)) {
    return 0;
  }

  int leavingFreeLook = (m_controlFlags & INPUT_FREE_LOOK_MASK) && !(m_controlFlags & ~bit & INPUT_FREE_LOOK_MASK);
  if (leavingFreeLook) {
    CGCamera *camera = Camera();
    camera->SyncFreeLookFacing();
    camera->DisableFreeLook(sticky);
  }

  m_controlFlags &= ~bit;
  if (leavingFreeLook) {
    if (IsMouseDragging()) {
      m_releaseAction = INPUT_RELEASE_NONE;
      return 1;
    }

    CGWorldFrame *worldFrame = CGWorldFrame::GetActive();
    FATALASSERT(worldFrame);
    if (m_releaseAction == INPUT_RELEASE_SELECT) {
      worldFrame->PerformDefaultAction(MOUSE_BUTTON_LEFT, OsGetAsyncTimeMs());
    } else if (m_releaseAction == INPUT_RELEASE_ACTION) {
      worldFrame->PerformDefaultAction(MOUSE_BUTTON_RIGHT, OsGetAsyncTimeMs());
    }
  }

  return 1;
}

void CGInputControl::MovePlayer(unsigned long now, CGUnit_C *player) {
  int direction = (m_controlFlags & INPUT_MOVE_PLAYER_AUTORUN) != 0;
  if (m_controlFlags & INPUT_MOVE_PLAYER_FORWARD_KEY) {
    ++direction;
  }
  if ((m_controlFlags & INPUT_MOVE_AND_TURN_PLAYER) == INPUT_MOVE_AND_TURN_PLAYER) {
    ++direction;
  }
  if (m_controlFlags & INPUT_MOVE_PLAYER_BACKWARD_KEY) {
    --direction;
  }

  if (direction) {
    if (player->GetUnitData()->standState) {
      player->ChangeStandState(0);
    } else if (!(m_controlFlags & INPUT_MOVE_PLAYER_SENT)) {
      player->OnMoveStartLocal(now, direction > 0);
      m_controlFlags |= INPUT_MOVE_PLAYER_SENT;
    }
  } else if (m_controlFlags & INPUT_MOVE_PLAYER_SENT) {
    player->OnMoveStopLocal(now);
    m_controlFlags &= ~INPUT_MOVE_PLAYER_SENT;
  }
}

void CGInputControl::StrafePlayer(unsigned long now, CGUnit_C *player) {
  int direction = (m_controlFlags & INPUT_STRAFE_PLAYER_LEFT_KEY) != 0;
  if ((m_controlFlags & INPUT_TURN_PLAYER) && (m_controlFlags & INPUT_TURN_PLAYER_LEFT_KEY)) {
    ++direction;
  }
  if (m_controlFlags & INPUT_STRAFE_PLAYER_RIGHT_KEY) {
    --direction;
  }
  if ((m_controlFlags & INPUT_TURN_PLAYER) && (m_controlFlags & INPUT_TURN_PLAYER_RIGHT_KEY)) {
    --direction;
  }

  if (direction) {
    if (player->GetUnitData()->standState) {
      player->ChangeStandState(0);
    } else if (!(m_controlFlags & INPUT_STRAFE_PLAYER_SENT)) {
      player->OnStrafeStartLocal(now, direction > 0);
      m_controlFlags |= INPUT_STRAFE_PLAYER_SENT;
    }
  } else if (m_controlFlags & INPUT_STRAFE_PLAYER_SENT) {
    player->OnStrafeStopLocal(now);
    m_controlFlags &= ~INPUT_STRAFE_PLAYER_SENT;
  }
}

void CGInputControl::TurnPlayer(unsigned long now, CGUnit_C *player) {
  if (!(m_controlFlags & INPUT_TURN_PLAYER)) {
    int direction = (m_controlFlags & INPUT_TURN_PLAYER_LEFT_KEY) != 0;
    if (m_controlFlags & INPUT_TURN_PLAYER_RIGHT_KEY) {
      --direction;
    }
    if (direction) {
      if (player->GetUnitData()->standState) {
        player->ChangeStandState(0);
      } else if (!(m_controlFlags & INPUT_TURN_PLAYER_SENT)) {
        player->OnTurnStartLocal(now, direction > 0);
        m_controlFlags |= INPUT_TURN_PLAYER_SENT;
      }
    } else if (m_controlFlags & INPUT_TURN_PLAYER_SENT) {
      player->OnTurnStopLocal(now);
      m_controlFlags &= ~INPUT_TURN_PLAYER_SENT;
    }
  }
}

void CGInputControl::PitchPlayer(unsigned long now, CGUnit_C *player) {
  if (!(m_controlFlags & INPUT_TURN_PLAYER)) {
    int direction = (m_controlFlags & INPUT_PITCH_PLAYER_UP_KEY) != 0;
    if (m_controlFlags & INPUT_PITCH_PLAYER_DOWN_KEY) {
      --direction;
    }
    if (direction) {
      if (!(m_controlFlags & INPUT_PITCH_PLAYER_SENT)) {
        player->OnPitchStartLocal(now, direction > 0);
        m_controlFlags |= INPUT_PITCH_PLAYER_SENT;
      }
    } else if (m_controlFlags & INPUT_PITCH_PLAYER_SENT) {
      player->OnPitchStopLocal(now);
      m_controlFlags &= ~INPUT_PITCH_PLAYER_SENT;
    }
  }
}

int CGInputControl::CameraCanTurnPlayer() const {
  CGPlayer_C *player = static_cast<CGPlayer_C *>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__));
  if (!player) {
    return 0;
  }

  const CGUnitData *unit = player->GetUnitData();
  bool              canIssueMovement = (unit->flags & 0x1000000) || ((player->GetType() & TYPE_PLAYER) && !unit->charm &&
                                                                     ((unit->flags & 2) || !(unit->flags & 0xC00004)) && !(unit->flags & 1));
  if (unit->health <= 0 || !canIssueMovement || (unit->flags & 0x40000) || player->IsInStandSitTransition() || unit->standState) {
    return 0;
  }

  CGCamera *camera = Camera();
  if (camera->m_target != player->GetGUID() || !(camera->m_flags & 8)) {
    return 0;
  }
  return (m_controlFlags & INPUT_TURN_PLAYER) != 0;
}

void CGInputControl::CameraTurnPlayer(unsigned long timestamp, float yaw, float pitch, bool setSmoothFacing) {
  if (!CameraCanTurnPlayer()) {
    return;
  }

  CGUnit_C *activeMover = static_cast<CGUnit_C *>(ClntObjMgrObjectPtr(CGUnit_C::m_activeMover, __FILE__, __LINE__));
  if (!activeMover) {
    return;
  }

  if (setSmoothFacing) {
    activeMover->SetSmoothFacing(yaw);
    return;
  }

  activeMover->OnSetRawFacingLocal(timestamp, yaw);
  if (activeMover->m_move.m_moveFlags & 0x2000000) {
    activeMover->OnSetPitchLocal(timestamp, 6.2831855f - pitch);
  }
  m_controlFlags &= 0xFFFF3FFF;
}
