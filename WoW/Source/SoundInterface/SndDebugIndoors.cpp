#include <Base/Base.h>
#include <WowConst.h>
#include <MapDefs.h>

#include "SoundInterface.h"

#include "Console/ConsoleClient.h"
#include "Object/ObjectClient/Object_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WorldClient/World.h"

#include <stdio.h>
#include <storm.h>

struct CHUNKHASHOBJ : public TSHashObject<CHUNKHASHOBJ, HASHKEY_STRI> {
  CHUNKHASHOBJ() {
  }

  CHUNKHASHOBJ(const CHUNKHASHOBJ &rhs);

  char                      zoneName[128];
  char                      subZoneName[128];
  UINT                      chunkNumber;
  _FSOUND_REVERB_PROPERTIES desc;

  void DumpInfo(int summary, int newlyCreated);
  void PrintInfo(FILE *outFile);
};

static UINT                                    s_currentChunk;
static TSHashTable<CHUNKHASHOBJ, HASHKEY_STRI> s_chunkHash;
static TSGrowableArray<CHUNKHASHOBJ *>         s_chunkList;

void CHUNKHASHOBJ::PrintInfo(FILE *outFile) {
  FATALASSERT(outFile);

  fprintf(outFile, "%s_%s\n", zoneName, subZoneName);
  fprintf(outFile, "%d\n", desc.Environment);
  fprintf(outFile, "%0.4f\n", desc.DecayTime);
  fprintf(outFile, "%0.4f\n", desc.EnvSize);
  fprintf(outFile, "%0.4f\n", desc.EnvDiffusion);
  fprintf(outFile, "%d\n", desc.Room);
  fprintf(outFile, "%d\n", desc.RoomHF);
  fprintf(outFile, "%0.4f\n", desc.DecayHFRatio);
  fprintf(outFile, "%d\n", desc.Reflections);
  fprintf(outFile, "%0.4f\n", desc.ReflectionsDelay);
  fprintf(outFile, "%d\n", desc.Reverb);
  fprintf(outFile, "%0.4f\n", desc.ReverbDelay);
  fprintf(outFile, "%0.4f\n", desc.RoomRolloffFactor);
  fprintf(outFile, "%0.4f\n", desc.AirAbsorptionHF);
  fprintf(outFile, "%d\n", desc.RoomLF);
  fprintf(outFile, "%0.4f\n", desc.DecayLFRatio);
  fprintf(outFile, "%0.4f\n", desc.EchoTime);
  fprintf(outFile, "%0.4f\n", desc.EchoDepth);
  fprintf(outFile, "%0.4f\n", desc.ModulationTime);
  fprintf(outFile, "%0.4f\n", desc.ModulationDepth);
  fprintf(outFile, "%0.4f\n", desc.HFReference);
  fprintf(outFile, "%0.4f\n", desc.LFReference);
  fprintf(outFile, "%d\n", desc.Environment);
}

void CHUNKHASHOBJ::DumpInfo(int summary, int newlyCreated) {
  if (newlyCreated) {
    ConsoleWrite("Created:", DEFAULT_COLOR);
  }

  ConsolePrintf("[%03d]   \"%s_%s\" %s", chunkNumber, zoneName, subZoneName, chunkNumber == s_currentChunk ? "[CURRENT]" : "");
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

int SndDebugListChunksINDOORS(LPCSTR command, LPCSTR arguments) {
  UINT i;
  for (i = 0; i < s_chunkList.Count(); ++i) {
    ASSERT(s_chunkList[i]);
    s_chunkList[i]->DumpInfo(1, 0);
  }
  ConsolePrintf("listed %d entries", i);
  return 1;
}

int CreateChunkINDOORS(LPCSTR command, LPCSTR arguments) {
  CGObject_C *object = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!object) {
    ConsoleWrite("Error, can't locate player!", DEFAULT_COLOR);
    return 1;
  }

  DWORD worldObject = object->GetWorldObject();
  if (!worldObject) {
    ConsoleWrite("Error, can't locate player world object!", DEFAULT_COLOR);
    return 1;
  }
  if (!CWorld::QueryObjectInside(worldObject)) {
    ConsoleWrite("Error, you don't seem to be indoors!", DEFAULT_COLOR);
    return 1;
  }

  LPCSTR subZoneName = CWorld::QueryChunkName();
  LPCSTR zoneName = 0;
  CWorld::QueryMapObjFileName(worldObject, zoneName);
  ASSERT(zoneName && *zoneName);
  LPCSTR slash = SStrChrR(zoneName, '\\');
  if (slash) {
    zoneName = slash + 1;
  }

  char hashName[256];
  SStrPrintf(hashName, sizeof(hashName), "%s%s", subZoneName, zoneName);
  CHUNKHASHOBJ *chunk = s_chunkHash.Ptr(SStrHashHT(hashName), hashName);
  if (!chunk) {
    chunk = s_chunkHash.New(SStrHashHT(hashName), hashName, 0, 0);
    *s_chunkList.New() = chunk;
    s_currentChunk = s_chunkList.Count() - 1;
    chunk->chunkNumber = s_currentChunk;
    SStrPrintf(chunk->zoneName, sizeof(chunk->zoneName), "%s", zoneName);
    SStrPrintf(chunk->subZoneName, sizeof(chunk->subZoneName), "%s", subZoneName);
    chunk->DumpInfo(1, 1);
  } else {
    s_currentChunk = chunk->chunkNumber;
    chunk->DumpInfo(1, 0);
  }
  return 1;
}

int SetCurrentChunkINDOORS(LPCSTR command, LPCSTR arguments) {
  if (arguments && *arguments) {
    UINT chunk = SStrToUnsigned(arguments);
    if (chunk < s_chunkList.Count()) {
      s_currentChunk = chunk;
      ConsolePrintf("Current chunk set to %d", chunk);
    } else {
      ConsoleWrite("Error, invalid chunk # specified!", DEFAULT_COLOR);
    }
  } else {
    ConsoleWrite("Error, this command needs a parameter, use \"ShowCurrentChunk\" to list chunks.", DEFAULT_COLOR);
  }
  return 1;
}

int ShowCurrentChunkINDOORS(LPCSTR command, LPCSTR arguments) {
  if (!s_chunkList.Count()) {
    ConsoleWrite("No chunks created!", DEFAULT_COLOR);
  } else if (s_currentChunk >= s_chunkList.Count()) {
    ConsoleWrite("Error, the current chunk is invalid!", DEFAULT_COLOR);
  } else {
    ASSERT(s_chunkList[s_currentChunk]);
    s_chunkList[s_currentChunk]->DumpInfo(0, 0);
  }
  return 1;
}

int SetChunkPropertyINDOORS(LPCSTR command, LPCSTR arguments) {
  if (s_currentChunk > s_chunkList.Count()) {
    ConsoleWrite("Error, the current chunk is invalid!", DEFAULT_COLOR);
    return 1;
  }

  CHUNKHASHOBJ *chunk = s_chunkList[s_currentChunk];
  ASSERT(chunk);
  UINT  prefNumber;
  float value;
  sscanf(arguments, "%d %f", &prefNumber, &value);
  int intValue = static_cast<int>(value);

  switch (prefNumber) {
    case 1:
    case 22:
      chunk->desc.Environment = intValue;
      break;
    case 2:
      chunk->desc.DecayTime = value;
      break;
    case 3:
      chunk->desc.EnvSize = value;
      break;
    case 4:
      chunk->desc.EnvDiffusion = value;
      break;
    case 5:
      chunk->desc.Room = intValue;
      break;
    case 6:
      chunk->desc.RoomHF = intValue;
      break;
    case 7:
      chunk->desc.DecayHFRatio = value;
      break;
    case 8:
      chunk->desc.Reflections = intValue;
      break;
    case 9:
      chunk->desc.ReflectionsDelay = value;
      break;
    case 10:
      chunk->desc.Reverb = intValue;
      break;
    case 11:
      chunk->desc.ReverbDelay = value;
      break;
    case 12:
      chunk->desc.RoomRolloffFactor = value;
      break;
    case 13:
      chunk->desc.AirAbsorptionHF = value;
      break;
    case 14:
      chunk->desc.RoomLF = intValue;
      break;
    case 15:
      chunk->desc.DecayLFRatio = value;
      break;
    case 16:
      chunk->desc.EchoTime = value;
      break;
    case 17:
      chunk->desc.EchoDepth = value;
      break;
    case 18:
      chunk->desc.ModulationTime = value;
      break;
    case 19:
      chunk->desc.ModulationDepth = value;
      break;
    case 20:
      chunk->desc.HFReference = value;
      break;
    case 21:
      chunk->desc.LFReference = value;
      break;
    default:
      ConsolePrintf("Error, unregonized property %d!", prefNumber);
      break;
  }

  SndInterfaceSetProviderPrefs(chunk->desc, chunk->desc);
  return 1;
}

int DumpChunksINDOORS(LPCSTR command, LPCSTR arguments) {
  UINT chunks = s_chunkList.Count();
  if (!chunks) {
    ConsoleWrite("Error, no chunk information to dump!", DEFAULT_COLOR);
    return 1;
  }

  FILE *outFile = 0;
  char  buffer[128];
  for (UINT fileNumber = 0; fileNumber < 100; ++fileNumber) {
    SStrPrintf(buffer, sizeof(buffer), "SndEAXChunkInfo_INDOORS_%02d.txt", fileNumber);
    outFile = fopen(buffer, "wt");
    if (outFile) {
      break;
    }
  }
  if (!outFile) {
    ConsoleWrite("Error, cannot create output file!", DEFAULT_COLOR);
    return 1;
  }

  fprintf(outFile, "%d\n", chunks);
  for (UINT i = 0; i < chunks; ++i) {
    ASSERT(s_chunkList[i]);
    s_chunkList[i]->PrintInfo(outFile);
  }
  fclose(outFile);
  return 1;
}

void IndoorsShutdown() {
  s_chunkHash.Clear();
  s_chunkList.Clear();
}
