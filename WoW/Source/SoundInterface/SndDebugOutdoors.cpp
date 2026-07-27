#include "SoundInterface.h"

#include "WorldClient/AreaListHashKey.h"

#include "Console/ConsoleClient.h"
#include "Object/ObjectClient/Object_C.h"
#include "ObjectMgrClient/ObjectMgrClient.h"
#include "WorldClient/World.h"

#include <stdio.h>
#include <storm.h>

struct OUTDOORSCHUNKHASHOBJ : public TSHashObject<OUTDOORSCHUNKHASHOBJ, AREAHASHKEY> {
  unsigned int              chunkNumber;
  _FSOUND_REVERB_PROPERTIES desc;
  unsigned int              continentID;
  unsigned int              areaID;

  void DumpInfo(int summary, int newlyCreated);
  void PrintInfo(FILE *outFile);
};

static unsigned int                                   s_currentContinent;
static TSHashTable<OUTDOORSCHUNKHASHOBJ, AREAHASHKEY> s_chunkHash;
static TSGrowableArray<OUTDOORSCHUNKHASHOBJ *>        s_chunkList;
static unsigned int                                   s_currentChunk;

void OUTDOORSCHUNKHASHOBJ::DumpInfo(int summary, int newlyCreated) {
  if (newlyCreated) {
    ConsoleWrite("Created:", DEFAULT_COLOR);
  }

  ConsolePrintf(
      "[%03d]   %d \"c%dz%ds%d\" %s", chunkNumber, areaID, continentID, areaID >> 16, static_cast<unsigned short>(areaID),
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

  fprintf(outFile, "c%dz%ds%d\n", continentID, areaID >> 16, static_cast<unsigned short>(areaID));
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

int DumpChunksOUTDOORS(const char *command, const char *arguments) {
  unsigned int chunks = s_chunkList.Count();
  if (!chunks) {
    ConsoleWrite("Error, no chunk information to dump!", DEFAULT_COLOR);
    return 1;
  }

  FILE *outFile = 0;
  char  buffer[128];
  for (unsigned int fileNumber = 0; fileNumber < 100; ++fileNumber) {
    SStrPrintf(buffer, sizeof(buffer), "SndEAXChunkInfo_OUTDOORS_%02d.txt", fileNumber);
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
  for (unsigned int i = 0; i < chunks; ++i) {
    ASSERT(s_chunkList[i]);
    s_chunkList[i]->PrintInfo(outFile);
  }
  fclose(outFile);
  return 1;
}

int ShowCurrentChunkOUTDOORS(const char *command, const char *arguments) {
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

int SetChunkPropertyOUTDOORS(const char *command, const char *arguments) {
  if (s_currentChunk > s_chunkList.Count()) {
    ConsoleWrite("Error, the current chunk is invalid!", DEFAULT_COLOR);
    return 1;
  }

  OUTDOORSCHUNKHASHOBJ *chunk = s_chunkList[s_currentChunk];
  ASSERT(chunk);
  unsigned int prefNumber;
  float        value;
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
      ConsolePrintf("Error, unrecognized property %d!", prefNumber);
      break;
  }

  SndInterfaceSetProviderPrefs(chunk->desc, chunk->desc);
  return 1;
}

int SetCurrentChunkOUTDOORS(const char *command, const char *arguments) {
  if (arguments && *arguments) {
    unsigned int chunk = SStrToUnsigned(arguments);
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

int CreateChunkOUTDOORS(const char *command, const char *arguments) {
  CGObject_C *object = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), __FILE__, __LINE__);
  if (!object) {
    ConsoleWrite("Error, can't locate player!", DEFAULT_COLOR);
    return 1;
  }

  unsigned long worldObject = object->GetWorldObject();
  if (!worldObject) {
    ConsoleWrite("Error, can't locate player world object!", DEFAULT_COLOR);
    return 1;
  }
  if (CWorld::QueryObjectInside(worldObject)) {
    ConsoleWrite("Error, you don't seem to be outdoors!", DEFAULT_COLOR);
    return 1;
  }

  NTempest::C3Vector location = object->GetPosition();
  unsigned int       areaID = CWorld::QueryAreaId(location.x, location.y);
  AREAHASHKEY        key(s_currentContinent, areaID >> 16, static_cast<unsigned short>(areaID));

  OUTDOORSCHUNKHASHOBJ *chunk = s_chunkHash.Ptr(areaID, key);
  if (!chunk) {
    chunk = s_chunkHash.New(areaID, key, 0, 0);
    *s_chunkList.New() = chunk;
    s_currentChunk = s_chunkList.Count() - 1;
    chunk->chunkNumber = s_currentChunk;
    chunk->continentID = s_currentContinent;
    chunk->areaID = areaID;
    chunk->DumpInfo(1, 1);
  } else {
    s_currentChunk = chunk->chunkNumber;
    chunk->DumpInfo(1, 0);
  }
  return 1;
}

int SndDebugListChunksOUTDOORS(const char *command, const char *arguments) {
  unsigned int i;
  for (i = 0; i < s_chunkList.Count(); ++i) {
    ASSERT(s_chunkList[i]);
    s_chunkList[i]->DumpInfo(1, 0);
  }
  ConsolePrintf("listed %d entries", i);
  return 1;
}

void OutdoorsShutdown() {
  s_chunkHash.Clear();
  s_chunkList.Clear();
}

void SndDebugRegisterContinent(unsigned int continent) {
  s_currentContinent = continent;
}
