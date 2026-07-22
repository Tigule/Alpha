#pragma once

#include <storm.h>

typedef unsigned int   uint;
typedef unsigned short uint16;

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

void __fastcall         PropInitialize();
void __fastcall         PropDestroy();
HPROPCONTEXT __fastcall PropCreateContext();
void __fastcall         PropSelectContext(HPROPCONTEXT context);
HPROPCONTEXT __fastcall PropGetSelectedContext();
void __fastcall         PropDeleteContext(HPROPCONTEXT context);
void *__fastcall        PropGet(PROPERTY id);
void __fastcall         PropSet(PROPERTY id, void *value);

DWORD __fastcall OsTlsAlloc();
void __fastcall  OsTlsFree(DWORD index);
void *__fastcall OsTlsGetValue(DWORD index);
BOOL __fastcall  OsTlsSetValue(DWORD index, void *value);

void __fastcall BaseInitializeGlobal();
void __fastcall BaseDestroyGlobal();
void __fastcall BaseInitializeContext();
void __fastcall BaseDestroyContext();
