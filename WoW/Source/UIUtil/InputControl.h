#pragma once

#include <new>
#include <storm.h>

#include <stddef.h>

class CMouseEvent;
class CGUnit_C;

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

  static CGInputControl *__fastcall GetActive();

  void OnUpdate(float elapsedSec);
  void OnMouseMove(const CMouseEvent &event);
  void OnMouseMoveRel(const CMouseEvent &evt);
  void Reset();
  void UpdatePlayer(unsigned long now);
  void SetReleaseAction(CGInputReleaseAction action);
  void SetControlBit(INPUT_CONTROL bit, int set, unsigned long now, int sticky);
  int  UnsetControlBit(INPUT_CONTROL bit, int sticky);
  int  CameraCanTurnPlayer() const;
  void CameraTurnPlayer(unsigned long timestamp, float yaw, float pitch, unsigned int setSmoothFacing);
  int  IsMouseDragMoving() const;

  static CGInputControl *s_inputControl;

  unsigned long        m_initializeTime;
  unsigned int         m_controlFlags;
  float                m_mouseChangeX;
  float                m_mouseChangeY;
  unsigned int         m_lastFrameMouseMoved;
  unsigned long        m_mouseDownTime;
  CGInputReleaseAction m_releaseAction;

 private:
  int  SetControlBit(INPUT_CONTROL bit);
  void MovePlayer(unsigned long now, CGUnit_C *player);
  void StrafePlayer(unsigned long now, CGUnit_C *player);
  void TurnPlayer(unsigned long now, CGUnit_C *player);
  void PitchPlayer(unsigned long now, CGUnit_C *player);
  int  IsMouseDragging() const;
};

void __fastcall InputControlInitialize();
void __fastcall InputControlRegisterScriptFunctions();
void __fastcall InputControlUnregisterScriptFunctions();
void __fastcall InputControlDestroy();
