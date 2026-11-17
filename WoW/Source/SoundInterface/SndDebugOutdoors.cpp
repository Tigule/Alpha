#include <Base/Base.h>
#include "SoundInterface.h"
#include <Gx/Gx.h>
#include <WowConst.h>
#include <Gx/CGxDevice.h>
#include "Object/ObjectClient/Unit_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include <MapDefs.h>
#include "WorldClient/World.h"

#include "WorldClient/AreaListHashKey.h"

#include "Console/ConsoleClient.h"
#include "Object/ObjectClient/Object_C.h"

#include <stdio.h>
#include <storm.h>

struct OUTDOORSCHUNKHASHOBJ : public TSHashObject<OUTDOORSCHUNKHASHOBJ, AREAHASHKEY> {
  UINT                      chunkNumber;
  _FSOUND_REVERB_PROPERTIES desc;
  UINT                      continentID;
  UINT                      areaID;

  void DumpInfo(int summary, int newlyCreated);
  void PrintInfo(FILE *outFile);
};

static UINT                                           s_currentContinent;
static TSHashTable<OUTDOORSCHUNKHASHOBJ, AREAHASHKEY> s_chunkHash;
static TSGrowableArray<OUTDOORSCHUNKHASHOBJ *>        s_chunkList;
static UINT                                           s_currentChunk;

void OUTDOORSCHUNKHASHOBJ::DumpInfo(int summary, int newlyCreated) {
  if (newlyCreated) {
    ConsoleWrite("Created:", DEFAULT_COLOR);
  }

  ConsolePrintf(
      "[%03d]   %d \"c%dz%ds%d\" %s", chunkNumber, areaID, continentID, areaID >> 16, areaID & 0xFFFF,
      chunkNumber == s_currentChunk ? "[CURRENT]" : ""
  );
  if (!summary) {
    ConsolePrintf("[%02d]: \"%s\": %d", 1, "Environment", desc.Environment);
    ConsolePrintf("[%02d]: \"%s\": %g", 2, "DecayTime", desc.DecayTime);
    ConsolePrintf("[%02d]: \"%s\": %g", 3, "EnvSize", desc.EnvSize);
    ConsolePrintf("[%02d]: \"%s\": %g", 4, "EnvDiffusion", desc.EnvDiffusion);
    ConsolePrintf("[%02d]: \"%s\": %d", 5, "Room", desc.Room);
    ConsolePrintf("[%02d]: \"%s\": %d", 6, "RoomHF", desc.RoomHF);
    ConsolePrintf("[%02d]: \"%s\": %g", 7, "DecayHFRatio", desc.DecayHFRatio);
    ConsolePrintf("[%02d]: \"%s\": %d", 8, "Reflections", desc.Reflections);
    ConsolePrintf("[%02d]: \"%s\": %g", 9, "ReflectionsDelay", desc.ReflectionsDelay);
    ConsolePrintf("[%02d]: \"%s\": %d", 10, "Reverb", desc.Reverb);
    ConsolePrintf("[%02d]: \"%s\": %g", 11, "ReverbDelay", desc.ReverbDelay);
    ConsolePrintf("[%02d]: \"%s\": %g", 12, "RoomRolloffFactor", desc.RoomRolloffFactor);
    ConsolePrintf("[%02d]: \"%s\": %g", 13, "AirAbsorptionHF", desc.AirAbsorptionHF);
    ConsolePrintf("[%02d]: \"%s\": %d", 14, "RoomLF", desc.RoomLF);
    ConsolePrintf("[%02d]: \"%s\": %g", 15, "DecayLFRatio", desc.DecayLFRatio);
    ConsolePrintf("[%02d]: \"%s\": %g", 16, "EchoTime", desc.EchoTime);
    ConsolePrintf("[%02d]: \"%s\": %g", 17, "EchoDepth", desc.EchoDepth);
    ConsolePrintf("[%02d]: \"%s\": %g", 18, "ModulationTime", desc.ModulationTime);
    ConsolePrintf("[%02d]: \"%s\": %g", 19, "ModulationDepth", desc.ModulationDepth);
    ConsolePrintf("[%02d]: \"%s\": %g", 20, "HFReference", desc.HFReference);
    ConsolePrintf("[%02d]: \"%s\": %g", 21, "LFReference", desc.LFReference);
    ConsolePrintf("[%02d]: \"%s\": %d", 22, "Environment", desc.Environment);
  }
}

void OUTDOORSCHUNKHASHOBJ::PrintInfo(FILE *outFile) {
  FATALASSERT(outFile);

  fprintf(outFile, "c%dz%ds%d\n", continentID, areaID >> 16, areaID & 0xFFFF);
#define SNDDEBUG_INT(index, name) fprintf(outFile, "%d\n", desc.name);
#define SNDDEBUG_FLOAT(index, name) fprintf(outFile, "%0.4f\n", desc.name);
#include "SndDebugCompiles.h"
#undef SNDDEBUG_INT
#undef SNDDEBUG_FLOAT
}

BOOL DumpChunksOUTDOORS(LPCSTR command, LPCSTR arguments) {
  UINT chunks = s_chunkList.Count();
  if (!chunks) {
    ConsoleWrite("Error, no chunk info found!", DEFAULT_COLOR);
    return TRUE;
  }

  FILE *outFile = NULL;
  char  buffer[128];
  for (UINT fileNumber = 0; fileNumber < 100; ++fileNumber) {
    SStrPrintf(buffer, sizeof(buffer), "SndEAXChunkInfo_OUTDOORS_%02d.txt", fileNumber);
    outFile = fopen(buffer, "w+t");
    if (outFile) {
      break;
    }
  }
  if (!outFile) {
    ConsoleWrite("Error, cannot create output file!", DEFAULT_COLOR);
    return TRUE;
  }

  fprintf(outFile, "%d\n", chunks);
  for (UINT i = 0; i < chunks; ++i) {
    ASSERT(s_chunkList[i]);
    s_chunkList[i]->PrintInfo(outFile);
  }
  fclose(outFile);
  return TRUE;
}

BOOL ShowCurrentChunkOUTDOORS(LPCSTR command, LPCSTR arguments) {
  if (!s_chunkList.Count()) {
    ConsoleWrite("No chunks created, use \"SndDebugCreateChunk\" to create a chunk for your current position", DEFAULT_COLOR);
    return TRUE;
  }
  if (s_currentChunk >= s_chunkList.Count()) {
    ConsoleWrite("Error, the current chunk is greater than the number of chunks..something is wrong!", DEFAULT_COLOR);
    return TRUE;
  }
  OUTDOORSCHUNKHASHOBJ *obj = s_chunkList[s_currentChunk];
  ASSERT(obj);
  obj->DumpInfo(0, 0);
  return TRUE;
}

BOOL SetChunkPropertyOUTDOORS(LPCSTR command, LPCSTR arguments) {
  if (s_currentChunk > s_chunkList.Count()) {
    ConsoleWrite("Error: the current chunk is invalid, contact Jeff", DEFAULT_COLOR);
    return TRUE;
  }

  OUTDOORSCHUNKHASHOBJ *obj = s_chunkList[s_currentChunk];
  ASSERT(obj);
  UINT  prefNumber;
  float value;
  _FSOUND_REVERB_PROPERTIES *desc = &obj->desc;
  sscanf(arguments, "%d %f", &prefNumber, &value);
  int intValue = value;

  switch (prefNumber) {
    case 1:
    case 22:
      desc->Environment = intValue;
      break;
    case 2:
      desc->DecayTime = value;
      break;
    case 3:
      desc->EnvSize = value;
      break;
    case 4:
      desc->EnvDiffusion = value;
      break;
    case 5:
      desc->Room = intValue;
      break;
    case 6:
      desc->RoomHF = intValue;
      break;
    case 7:
      desc->DecayHFRatio = value;
      break;
    case 8:
      desc->Reflections = intValue;
      break;
    case 9:
      desc->ReflectionsDelay = value;
      break;
    case 10:
      desc->Reverb = intValue;
      break;
    case 11:
      desc->ReverbDelay = value;
      break;
    case 12:
      desc->RoomRolloffFactor = value;
      break;
    case 13:
      desc->AirAbsorptionHF = value;
      break;
    case 14:
      desc->RoomLF = intValue;
      break;
    case 15:
      desc->DecayLFRatio = value;
      break;
    case 16:
      desc->EchoTime = value;
      break;
    case 17:
      desc->EchoDepth = value;
      break;
    case 18:
      desc->ModulationTime = value;
      break;
    case 19:
      desc->ModulationDepth = value;
      break;
    case 20:
      desc->HFReference = value;
      break;
    case 21:
      desc->LFReference = value;
      break;
    default:
      ConsolePrintf("Error, unrecognized property %d!", prefNumber);
      break;
  }

  SndInterfaceSetProviderPrefs(*desc, *desc);
  return TRUE;
}

BOOL SetCurrentChunkOUTDOORS(LPCSTR command, LPCSTR arguments) {
  if (arguments && *arguments) {
    UINT chunk = SStrToUnsigned(arguments);
    if (chunk >= s_chunkList.Count()) {
      ConsoleWrite("Error, invalid chunk # specified!", DEFAULT_COLOR);
    } else {
      s_currentChunk = chunk;
      ConsolePrintf("Current chunk set to %d", chunk);
    }
  } else {
    ConsoleWrite("Error, this command needs a parameter, use \"ShowCurrentChunk\" to list chunks.", DEFAULT_COLOR);
  }
  return TRUE;
}

BOOL CreateChunkOUTDOORS(LPCSTR command, LPCSTR arguments) {
  CGObject_C *object = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!object) {
    ConsoleWrite("Error, can't locate current player!", DEFAULT_COLOR);
    return TRUE;
  }

  DWORD_PTR worldObject = object->GetWorldObject();
  if (!worldObject) {
    ConsoleWrite("Error, can't locate current player's worldobject!", DEFAULT_COLOR);
    return TRUE;
  }
  if (CWorld::QueryObjectInside(worldObject)) {
    ConsoleWrite("Error, you don't seem to be outdoors!", DEFAULT_COLOR);
    return TRUE;
  }

  NTempest::C3Vector location = object->GetPosition();
  UINT               areaID = CWorld::QueryAreaId(location.x, location.y);
  AREAHASHKEY        key(s_currentContinent, areaID >> 16, areaID & 0xFFFF);

  OUTDOORSCHUNKHASHOBJ *chunk = s_chunkHash.Ptr(areaID, key);
  if (chunk) {
    chunk->DumpInfo(1, 0);
    return TRUE;
  }

  chunk = s_chunkHash.New(areaID, key, 0, 0);
  UINT count = s_chunkList.Count();
  *s_chunkList.New() = chunk;
  s_currentChunk = count;
  chunk->chunkNumber = count;
  chunk->continentID = s_currentContinent;
  chunk->areaID = areaID;
  chunk->DumpInfo(1, 1);
  return TRUE;
}

BOOL SndDebugListChunksOUTDOORS(LPCSTR command, LPCSTR arguments) {
  UINT i;
  for (i = 0; i < s_chunkList.Count(); ++i) {
    OUTDOORSCHUNKHASHOBJ *obj = s_chunkList[i];
    ASSERT(obj);
    obj->DumpInfo(1, 0);
  }
  ConsolePrintf("listed %d entries", i);
  return TRUE;
}

void OutdoorsShutdown() {
  s_chunkHash.Clear();
  s_chunkList.Clear();
}

void SndDebugRegisterContinent(UINT continent) {
  s_currentContinent = continent;
}
