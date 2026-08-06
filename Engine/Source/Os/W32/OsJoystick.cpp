#include "OsJoystick.h"

#include <storm.h>
#include <stpl.h>
#include <mmsystem.h>

struct W32Joystick {
  struct _transaxis {
    int   offset;
    float scale;
  };

  UINT       id;
  _transaxis transaxis[6];
  JOYCAPS    caps;
};

static TSGrowableArray<W32Joystick> s_joystick;

int OsNumJoysticks() {
  JOYCAPS      joycaps;
  JOYINFOEX    joyinfo;
  int          maxdevs;
  int          id;
  W32Joystick *joystick;

  if (s_joystick.Count()) {
    return s_joystick.Count();
  }

  maxdevs = joyGetNumDevs();
  for (id = 0; id < maxdevs; ++id) {
    joyinfo.dwSize = sizeof(joyinfo);
    joyinfo.dwFlags = JOY_RETURNALL;

    if (joyGetPosEx(id, &joyinfo) != JOYERR_NOERROR) {
      continue;
    }

    if (joyGetDevCaps(id, &joycaps, sizeof(joycaps)) != JOYERR_NOERROR) {
      continue;
    }

    joystick = s_joystick.New();
    joystick->id = id;
    joystick->caps = joycaps;
  }

  return s_joystick.Count();
}

OsJoystickID OsOpenJoystick(int index) {
  JOYCAPS joycaps;
  int     axis_min[6];
  int     axis_max[6];
  int     caps_flags[4] = {
      JOYCAPS_HASZ,
      JOYCAPS_HASR,
      JOYCAPS_HASU,
      JOYCAPS_HASV,
  };
  int axis;

  if (!s_joystick.Count() && !OsNumJoysticks()) {
    return -1;
  }

  ASSERT((UINT)index < s_joystick.Count());

  joycaps = s_joystick[index].caps;

  axis_min[0] = joycaps.wXmin;
  axis_max[0] = joycaps.wXmax;
  axis_min[1] = joycaps.wYmin;
  axis_max[1] = joycaps.wYmax;
  axis_min[2] = joycaps.wZmin;
  axis_max[2] = joycaps.wZmax;
  axis_min[3] = joycaps.wRmin;
  axis_max[3] = joycaps.wRmax;
  axis_min[4] = joycaps.wUmin;
  axis_max[4] = joycaps.wUmax;
  axis_min[5] = joycaps.wVmin;
  axis_max[5] = joycaps.wVmax;

  for (axis = 0; axis < 6; ++axis) {
    if (axis < 2 || (joycaps.wCaps & caps_flags[axis - 2])) {
      s_joystick[index].transaxis[axis].offset = -32768 - axis_min[axis];
      s_joystick[index].transaxis[axis].scale = 65535.0f / (axis_max[axis] - axis_min[axis]);
    } else {
      s_joystick[index].transaxis[axis].offset = 0;
      s_joystick[index].transaxis[axis].scale = 1.0f;
    }
  }

  return index;
}

void OsCloseJoystick(OsJoystickID id) {
}

int OsGetNumButtons(OsJoystickID id) {
  return s_joystick[id].caps.wNumButtons;
}

int OsGetNumAxes(OsJoystickID id) {
  return s_joystick[id].caps.wNumAxes;
}

UINT OsGetButtonState(OsJoystickID id) {
  JOYINFOEX joyinfo;

  joyinfo.dwSize = sizeof(joyinfo);
  joyinfo.dwFlags = JOY_RETURNBUTTONS;

  if (joyGetPosEx(s_joystick[id].id, &joyinfo) == JOYERR_NOERROR) {
    return joyinfo.dwButtons;
  }

  return 0;
}

int OsGetButtonState(OsJoystickID id, int index) {
  return OsGetButtonState(id) & (1 << index);
}

int OsGetAxisState(OsJoystickID id, int index) {
  DWORD flags[6] = {
      JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, JOY_RETURNR, JOY_RETURNU, JOY_RETURNV,
  };
  JOYINFOEX                joyinfo;
  DWORD                    pos[6];
  W32Joystick::_transaxis *transaxis;

  joyinfo.dwSize = sizeof(joyinfo);
  joyinfo.dwFlags = flags[index];

  if (joyGetPosEx(s_joystick[id].id, &joyinfo) != JOYERR_NOERROR) {
    return 0;
  }

  pos[0] = joyinfo.dwXpos;
  pos[1] = joyinfo.dwYpos;
  pos[2] = joyinfo.dwZpos;
  pos[3] = joyinfo.dwRpos;
  pos[4] = joyinfo.dwUpos;
  pos[5] = joyinfo.dwVpos;

  transaxis = &s_joystick[id].transaxis[index];

  if (!(joyinfo.dwFlags & flags[index])) {
    return 0;
  }

  return static_cast<int>((static_cast<double>(pos[index]) + transaxis->offset) * transaxis->scale);
}
