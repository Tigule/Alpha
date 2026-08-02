#include "SoundInterface.h"

#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "Object/ObjectClient/Object_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"

#include "Event/EvtApi.h"
#include "Os/OsTime.h"

#include <stdio.h>
#include <storm.h>

static int  PingSound(const char *command, const char *arguments);
static int  RoomType(const char *command, const char *arguments);
static int  DebugTickHandler(const void *dataPtr, void *param);
static void SndDebugTick();

int DumpChunksINDOORS(const char *command, const char *arguments);
int ShowCurrentChunkINDOORS(const char *command, const char *arguments);
int SetChunkPropertyINDOORS(const char *command, const char *arguments);
int SetCurrentChunkINDOORS(const char *command, const char *arguments);
int CreateChunkINDOORS(const char *command, const char *arguments);
int SndDebugListChunksINDOORS(const char *command, const char *arguments);
int DumpChunksOUTDOORS(const char *command, const char *arguments);
int ShowCurrentChunkOUTDOORS(const char *command, const char *arguments);
int SetChunkPropertyOUTDOORS(const char *command, const char *arguments);
int SetCurrentChunkOUTDOORS(const char *command, const char *arguments);
int CreateChunkOUTDOORS(const char *command, const char *arguments);
int SndDebugListChunksOUTDOORS(const char *command, const char *arguments);
void SndDebugRegisterContinent(unsigned int continent);

static unsigned int       s_pingSound;
static unsigned int       s_pingFrequency;
static unsigned int       s_lastPingTime;
static NTempest::C3Vector s_pingPosition;

static int PingSound(const char *command, const char *arguments) {
  if (arguments && *arguments) {
    s_lastPingTime = 0;
    sscanf(arguments, "%d %d", &s_pingSound, &s_pingFrequency);

    if (!s_pingSound) {
      ConsolePrintf("Disabling ping sound");
      return 1;
    }

    if (s_pingFrequency <= 100) {
      s_pingFrequency = 100;
    }

    CGObject_C *object = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
    if (object) {
      s_pingPosition = object->GetPosition();
      s_pingPosition.z += 2.0f;
    } else {
      Sound::GetListenerPosition(s_pingPosition);
    }

    ConsolePrintf("Ping sound %d set for interval of %d milliseconds", s_pingSound, s_pingFrequency);
  }

  return 1;
}

static int RoomType(const char *command, const char *arguments) {
  SNDROOMTYPE roomType;

  if (!arguments || (roomType = static_cast<SNDROOMTYPE>(SStrToInt(arguments))) <= SNDROOMTYPE_PSYCHOTIC) {
    roomType = SNDROOMTYPE_PSYCHOTIC;
  }

  SndSetRoomType(roomType);
  return 1;
}

static void SndDebugTick() {
  if (s_pingSound) {
    int currentTime = static_cast<int>(OsGetAsyncTimeMs());

    if (currentTime >= static_cast<int>(s_lastPingTime + s_pingFrequency)) {
      CGObject_C *object = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
      if (object) {
        SndInterfacePlaySound(s_pingSound, s_pingPosition, -1, 1.0f);
        s_lastPingTime = currentTime;
      }
    }
  }
}

static int DebugTickHandler(const void *dataPtr, void *param) {
  SndDebugTick();
  return 1;
}

void SndDebugInitialize() {
  ConsoleCommandRegister("SndDebugPingSound", PingSound, DEBUG, 0);
  ConsoleCommandRegister("SndDebugRoomType", RoomType, DEBUG, 0);
  EventRegister(EVENT_ID_IDLE, DebugTickHandler);
}

void SndDebugShutdown() {
  ConsoleCommandUnregister("SndDebugPingSound");
  ConsoleCommandUnregister("SndDebugRoomType");
  IndoorsShutdown();
  OutdoorsShutdown();
  s_pingSound = 0;
  EventUnregister(EVENT_ID_IDLE, DebugTickHandler);
}

void SndDebugDungeonTransition(int indoors, unsigned int continent) {
  ConsoleCommandUnregister("SndDebugListChunks");
  ConsoleCommandUnregister("SndDebugCreateChunk");
  ConsoleCommandUnregister("SndDebugSetCurrentChunk");
  ConsoleCommandUnregister("SndDebugSetChunkProperty");
  ConsoleCommandUnregister("SndDebugShowCurrentChunk");
  ConsoleCommandUnregister("SndDebugDumpChunks");

  if (indoors) {
    ConsoleCommandRegister("SndDebugDumpChunks", DumpChunksINDOORS, DEBUG, 0);
    ConsoleCommandRegister("SndDebugShowCurrentChunk", ShowCurrentChunkINDOORS, DEBUG, 0);
    ConsoleCommandRegister("SndDebugSetChunkProperty", SetChunkPropertyINDOORS, DEBUG, 0);
    ConsoleCommandRegister("SndDebugSetCurrentChunk", SetCurrentChunkINDOORS, DEBUG, 0);
    ConsoleCommandRegister("SndDebugCreateChunk", CreateChunkINDOORS, DEBUG, 0);
    ConsoleCommandRegister("SndDebugListChunks", SndDebugListChunksINDOORS, DEBUG, 0);
  } else {
    SndDebugRegisterContinent(continent);
    ConsoleCommandRegister("SndDebugDumpChunks", DumpChunksOUTDOORS, DEBUG, 0);
    ConsoleCommandRegister("SndDebugShowCurrentChunk", ShowCurrentChunkOUTDOORS, DEBUG, 0);
    ConsoleCommandRegister("SndDebugSetChunkProperty", SetChunkPropertyOUTDOORS, DEBUG, 0);
    ConsoleCommandRegister("SndDebugSetCurrentChunk", SetCurrentChunkOUTDOORS, DEBUG, 0);
    ConsoleCommandRegister("SndDebugCreateChunk", CreateChunkOUTDOORS, DEBUG, 0);
    ConsoleCommandRegister("SndDebugListChunks", SndDebugListChunksOUTDOORS, DEBUG, 0);
  }
}
