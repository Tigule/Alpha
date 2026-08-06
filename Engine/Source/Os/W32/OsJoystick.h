#pragma once

#include <Base/Base.h>

typedef int OsJoystickID;

int          OsNumJoysticks();
OsJoystickID OsOpenJoystick(int index);
void         OsCloseJoystick(OsJoystickID id);
int          OsGetNumButtons(OsJoystickID id);
int          OsGetNumAxes(OsJoystickID id);
UINT         OsGetButtonState(OsJoystickID id);
int          OsGetButtonState(OsJoystickID id, int index);
int          OsGetAxisState(OsJoystickID id, int index);
