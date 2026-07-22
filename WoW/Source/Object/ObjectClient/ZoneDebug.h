#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_ZONEDEBUG_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_ZONEDEBUG_H

void __fastcall ZoneDebugInitialize();
void __fastcall ZoneDebugDestroy();
bool __fastcall ZoneDebugIsInCurrentZone(float x, float y);

#endif
