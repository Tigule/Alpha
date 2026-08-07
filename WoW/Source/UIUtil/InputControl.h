#pragma once

#include <new>
#include <storm.h>

#include <stddef.h>

class CMouseEvent;
class CGUnit_C;
class CGGameUI;

enum INPUT_CONTROL {
  INPUT_TURN_PLAYER = 0x00000001,
  INPUT_TURN_CAMERA = 0x00000002,
  INPUT_MOVE_PLAYER_OR_TURN_CAMERA = 0x00000004,
  INPUT_MOVE_PLAYER_FORWARD_KEY = 0x00000008,
  INPUT_MOVE_PLAYER_BACKWARD_KEY = 0x00000010,
  INPUT_STRAFE_PLAYER_LEFT_KEY = 0x00000020,
  INPUT_STRAFE_PLAYER_RIGHT_KEY = 0x00000040,
  INPUT_TURN_PLAYER_LEFT_KEY = 0x00000080,
  INPUT_TURN_PLAYER_RIGHT_KEY = 0x00000100,
  INPUT_PITCH_PLAYER_UP_KEY = 0x00000200,
  INPUT_PITCH_PLAYER_DOWN_KEY = 0x00000400,
  INPUT_MOVE_PLAYER_AUTORUN = 0x00000800,
  INPUT_MOVE_PLAYER_SENT = 0x00001000,
  INPUT_STRAFE_PLAYER_SENT = 0x00002000,
  INPUT_TURN_PLAYER_SENT = 0x00004000,
  INPUT_PITCH_PLAYER_SENT = 0x00008000,
  INPUT_PLAYER_MOVED = 0x10000000,
  INPUT_CAMERA_MOVED = 0x20000000,
  INPUT_FREE_LOOK_MASK = 0x00000007,
  INPUT_MOVE_PLAYER_MASK = 0x00000018,
  INPUT_STRAFE_PLAYER_MASK = 0x00000060,
  INPUT_TURN_PLAYER_MASK = 0x00000180,
  INPUT_PITCH_PLAYER_MASK = 0x00000600,
  INPUT_MOVE_AND_TURN_PLAYER = 0x00000005
};

enum CGInputReleaseAction {
  INPUT_RELEASE_NONE = 0,
  INPUT_RELEASE_SELECT = 1,
  INPUT_RELEASE_ACTION = 2
};

class CGInputControl {
 public:
  CGInputControl();

  static void Initialize() {
    ASSERT(!s_inputControl);
    s_inputControl = NEW(CGInputControl);
  }

  static void Destroy() {
    ASSERT(s_inputControl);
    DEL(s_inputControl);
    s_inputControl = 0;
  }

  static CGInputControl *GetActive();

  void OnUpdate(float elapsedSec);
  void OnMouseMove(const CMouseEvent &event);
  void OnMouseMoveRel(const CMouseEvent &evt);
  void Reset();
  void UpdatePlayer(DWORD now);
  void SetReleaseAction(CGInputReleaseAction action);
  void SetControlBit(INPUT_CONTROL bit, int set, DWORD now, int sticky);
  BOOL CameraCanTurnPlayer() const;
  void CameraTurnPlayer(DWORD timestamp, float yaw, float pitch, bool setSmoothFacing);
  BOOL IsMovingForward() const;
  BOOL IsAutoRunning() const {
    return (m_controlFlags & INPUT_MOVE_PLAYER_AUTORUN) != 0;
  }
  BOOL IsFreeLooking() const;
  BOOL IsMouseDragMoving() const;
  BOOL HasPlayerMoved() const {
    return (m_controlFlags & INPUT_PLAYER_MOVED) != 0;
  }
  BOOL HasCameraMoved() const {
    return (m_controlFlags & INPUT_CAMERA_MOVED) != 0;
  }
  DWORD GetInitializeTime() const {
    return m_initializeTime;
  }

 private:
  friend class CGGameUI;

  static CGInputControl *s_inputControl;

  DWORD                m_initializeTime;
  UINT                 m_controlFlags;
  float                m_mouseChangeX;
  float                m_mouseChangeY;
  UINT                 m_lastFrameMouseMoved;
  DWORD                m_mouseDownTime;
  CGInputReleaseAction m_releaseAction;

  BOOL SetControlBit(INPUT_CONTROL bit);
  BOOL UnsetControlBit(INPUT_CONTROL bit, int sticky);
  void MovePlayer(DWORD now, CGUnit_C *player);
  void StrafePlayer(DWORD now, CGUnit_C *player);
  void TurnPlayer(DWORD now, CGUnit_C *player);
  void PitchPlayer(DWORD now, CGUnit_C *player);
  BOOL IsMouseDragging() const;
};

void InputControlInitialize();
void InputControlRegisterScriptFunctions();
void InputControlUnregisterScriptFunctions();
void InputControlDestroy();
