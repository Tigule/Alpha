#pragma once

typedef int OsJoystickID;

int __fastcall          OsNumJoysticks();
OsJoystickID __fastcall OsOpenJoystick(int index);
void __fastcall         OsCloseJoystick(OsJoystickID id);
int __fastcall          OsGetNumButtons(OsJoystickID id);
int __fastcall          OsGetNumAxes(OsJoystickID id);
unsigned int __fastcall OsGetButtonState(OsJoystickID id);
int __fastcall          OsGetButtonState(OsJoystickID id, int index);
int __fastcall          OsGetAxisState(OsJoystickID id, int index);
