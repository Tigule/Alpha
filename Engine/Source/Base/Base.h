#pragma once

#include <storm.h>

const float PI = 3.14159265358979323846f;
const float TWO_PI = PI + PI;
const float OO_TWO_PI = 1.0f / TWO_PI;

DECLARE_STRICT_HANDLE(HPROPCONTEXT);

enum PROPERTY {
  PROP_EVENTCONTEXT = 0x00,
  PROP_HANDLETABLE = 0x01,
  PROP_EVENTSTATE = 0x02,
  PROP_TIMERS = 0x03,
  PROP_STRINGSTATE = 0x04,
  PROP_JASS = 0x05,
  PROP_JVM = 0x06,
  PROP_TEXTURES = 0x07,
  PROP_MODELS = 0x08,
  PROP_AGILE = 0x09,
  PROP_TEMPEST = 0x0A,
  PROP_RAIN = 0x0B,
  PROP_IPSE = 0x0C,
  PROP_APPLICATION = 0x0D,
  PROP_NET = 0x0E,
  PROP_WORLD = 0x0F,
  PROP_BATTLENET = 0x10,
  PROP_COLLISION = 0x11,
  PROP_AUTHENTICATION = 0x12,
  PROPERTIES = 0x13
};

void         PropInitialize();
void         PropDestroy();
HPROPCONTEXT PropCreateContext();
void         PropSelectContext(HPROPCONTEXT context);
HPROPCONTEXT PropGetSelectedContext();
void         PropDeleteContext(HPROPCONTEXT context);
LPVOID       PropGet(PROPERTY id);
void         PropSet(PROPERTY id, LPVOID value);

DWORD  OsTlsAlloc();
void   OsTlsFree(DWORD index);
LPVOID OsTlsGetValue(DWORD index);
BOOL   OsTlsSetValue(DWORD index, LPVOID value);

void BaseInitializeGlobal();
void BaseDestroyGlobal();
void BaseInitializeContext();
void BaseDestroyContext();
