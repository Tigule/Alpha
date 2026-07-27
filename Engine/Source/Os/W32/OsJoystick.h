#pragma once

typedef int OsJoystickID;

int OsNumJoysticks();
OsJoystickID OsOpenJoystick(int index);
void OsCloseJoystick(OsJoystickID id);
int OsGetNumButtons(OsJoystickID id);
int OsGetNumAxes(OsJoystickID id);
unsigned int OsGetButtonState(OsJoystickID id);
int OsGetButtonState(OsJoystickID id, int index);
int OsGetAxisState(OsJoystickID id, int index);
