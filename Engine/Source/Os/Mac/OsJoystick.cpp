#include <Base/Base.h>

#include "Os/W32/OsJoystick.h"

OsJoystickID OsOpenJoystick(int index) {
  return -1;
}

void OsCloseJoystick(OsJoystickID id) {
}

unsigned int OsGetButtonState(OsJoystickID id) {
  return 0;
}

int OsGetAxisState(OsJoystickID id, int index) {
  return 0;
}
