#include "SoundInterface.h"

#include "Console/ConsoleClient.h"
#include "Console/ConsoleCommand.h"
#include "DB/DBClient/DBClient.h"
#include "DB/DBClient/AutoCode/SoundProviderPreferencesRec.h"

#include "Event/EvtApi.h"
#include "Os/OsTime.h"
#include "Os/W32/OsSound.h"

#include <storm.h>

static int EnvironmentHandler(const char *command, const char *arguments);
static int EnvironmentListHandler(const char *command, const char *arguments);
static float InterpFloat(float progress, float start, float end);
static int InterpInt(float progress, int start, int end);
static void StopWorldIdleHandler();
static int WorldIdleHandler(const void *dataPtr, void *param);
static void StartProviderPrefFade(const _FSOUND_REVERB_PROPERTIES &rec, unsigned int duration);
static void StartWorldIdleHandler();
static void StopProviderPrefFade();
static void SaveDesc(_FSOUND_REVERB_PROPERTIES &desc, const SoundProviderPreferencesRec *rec);

static unsigned int              s_providerPrefFadeStartTime;
static unsigned int              s_providerPrefFadeEndTime;
static unsigned int              s_providerPrefFadeDuration;
static _FSOUND_REVERB_PROPERTIES s_startProviderDesc;
static _FSOUND_REVERB_PROPERTIES s_currentProviderDesc;
static _FSOUND_REVERB_PROPERTIES s_targetProviderDesc;
static _FSOUND_REVERB_PROPERTIES s_desc;
static _FSOUND_REVERB_PROPERTIES s_descUnderwater;
static int                       s_flags;
static bool                      s_idleRunning;

static float InterpFloat(float progress, float start, float end) {
  return start + progress * (end - start);
}

static int InterpInt(float progress, int start, int end) {
  return static_cast<int>(start + progress * (end - start));
}

void SndInterfaceFadeProviderPrefs(const EVENT_DATA_IDLE *data) {
  if (static_cast<int>(data->time) > static_cast<int>(s_providerPrefFadeEndTime)) {
    s_currentProviderDesc = s_targetProviderDesc;
    StopWorldIdleHandler();
    Sound::SetReverbProperties(&s_currentProviderDesc);
    return;
  }

  float progress = 0.0f;
  if (s_providerPrefFadeDuration) {
    progress = static_cast<float>(static_cast<int>(data->time - s_providerPrefFadeStartTime)) / static_cast<int>(s_providerPrefFadeDuration);
  } else {
    progress = 1.0f;
  }

  progress = min(max(progress, 0.0f), 1.0f);

  s_currentProviderDesc.DecayTime = InterpFloat(progress, s_startProviderDesc.DecayTime, s_targetProviderDesc.DecayTime);
  s_currentProviderDesc.EnvSize = InterpFloat(progress, s_startProviderDesc.EnvSize, s_targetProviderDesc.EnvSize);
  s_currentProviderDesc.EnvDiffusion = InterpFloat(progress, s_startProviderDesc.EnvDiffusion, s_targetProviderDesc.EnvDiffusion);
  s_currentProviderDesc.Room = InterpInt(progress, s_startProviderDesc.Room, s_targetProviderDesc.Room);
  s_currentProviderDesc.RoomHF = InterpInt(progress, s_startProviderDesc.RoomHF, s_targetProviderDesc.RoomHF);
  s_currentProviderDesc.DecayHFRatio = InterpFloat(progress, s_startProviderDesc.DecayHFRatio, s_targetProviderDesc.DecayHFRatio);
  s_currentProviderDesc.Reflections = InterpInt(progress, s_startProviderDesc.Reflections, s_targetProviderDesc.Reflections);
  s_currentProviderDesc.ReflectionsDelay = InterpFloat(progress, s_startProviderDesc.ReflectionsDelay, s_targetProviderDesc.ReflectionsDelay);
  s_currentProviderDesc.Reverb = InterpInt(progress, s_startProviderDesc.Reverb, s_targetProviderDesc.Reverb);
  s_currentProviderDesc.ReverbDelay = InterpFloat(progress, s_startProviderDesc.ReverbDelay, s_targetProviderDesc.ReverbDelay);
  s_currentProviderDesc.RoomRolloffFactor = InterpFloat(progress, s_startProviderDesc.RoomRolloffFactor, s_targetProviderDesc.RoomRolloffFactor);
  s_currentProviderDesc.AirAbsorptionHF = InterpFloat(progress, s_startProviderDesc.AirAbsorptionHF, s_targetProviderDesc.AirAbsorptionHF);
  s_currentProviderDesc.RoomLF = InterpInt(progress, s_startProviderDesc.RoomLF, s_targetProviderDesc.RoomLF);
  s_currentProviderDesc.DecayLFRatio = InterpFloat(progress, s_startProviderDesc.DecayLFRatio, s_targetProviderDesc.DecayLFRatio);
  s_currentProviderDesc.EchoTime = InterpFloat(progress, s_startProviderDesc.EchoTime, s_targetProviderDesc.EchoTime);
  s_currentProviderDesc.EchoDepth = InterpFloat(progress, s_startProviderDesc.EchoDepth, s_targetProviderDesc.EchoDepth);
  s_currentProviderDesc.ModulationTime = InterpFloat(progress, s_startProviderDesc.ModulationTime, s_targetProviderDesc.ModulationTime);
  s_currentProviderDesc.ModulationDepth = InterpFloat(progress, s_startProviderDesc.ModulationDepth, s_targetProviderDesc.ModulationDepth);
  s_currentProviderDesc.HFReference = InterpFloat(progress, s_startProviderDesc.HFReference, s_targetProviderDesc.HFReference);
  s_currentProviderDesc.LFReference = InterpFloat(progress, s_startProviderDesc.LFReference, s_targetProviderDesc.LFReference);

  Sound::SetReverbProperties(&s_currentProviderDesc);
}

static int WorldIdleHandler(const void *dataPtr, void *param) {
  SndInterfaceFadeProviderPrefs(static_cast<const EVENT_DATA_IDLE *>(dataPtr));
  return 1;
}

static void StartWorldIdleHandler() {
  EventRegister(EVENT_ID_IDLE, WorldIdleHandler);
  s_idleRunning = true;
}

static void StopWorldIdleHandler() {
  EventUnregister(EVENT_ID_IDLE, WorldIdleHandler);
  s_idleRunning = false;
}

static void StartProviderPrefFade(const _FSOUND_REVERB_PROPERTIES &rec, unsigned int duration) {
  unsigned int startTime = OsGetAsyncTimeMs();

  s_providerPrefFadeDuration = duration;
  s_providerPrefFadeStartTime = startTime;
  s_providerPrefFadeEndTime = startTime + duration;
  s_startProviderDesc = s_currentProviderDesc;
  s_targetProviderDesc = rec;
  StartWorldIdleHandler();
}

static void StopProviderPrefFade() {
  StopWorldIdleHandler();
  Sound::SetReverbProperties(&s_currentProviderDesc);
}

static void SaveDesc(_FSOUND_REVERB_PROPERTIES &desc, const SoundProviderPreferencesRec *rec) {
  desc.Environment = rec->m_EAXEnvironmentSelection;
  desc.EnvSize = rec->m_EAX2EnvironmentSize;
  desc.EnvDiffusion = rec->m_EAX2EnvironmentDiffusion;
  desc.Room = rec->m_EAX2Room;
  desc.RoomHF = rec->m_EAX2RoomHF;
  desc.RoomLF = rec->m_EAX3RoomLF;
  desc.DecayTime = rec->m_EAXDecayTime;
  desc.DecayHFRatio = rec->m_EAX2DecayHFRatio;
  desc.DecayLFRatio = rec->m_EAX3DecayLFRatio;
  desc.Reflections = rec->m_EAX2Reflections;
  desc.ReflectionsDelay = rec->m_EAX2ReflectionsDelay;
  desc.Reverb = rec->m_EAX2Reverb;
  desc.ReverbDelay = rec->m_EAX2ReverbDelay;
  desc.EchoTime = rec->m_EAX3EchoTime;
  desc.EchoDepth = rec->m_EAX3EchoDepth;
  desc.ModulationTime = rec->m_EAX3ModulationTime;
  desc.ModulationDepth = rec->m_EAX3ModulationDepth;
  desc.AirAbsorptionHF = rec->m_EAX2AirAbsorption;
  desc.HFReference = rec->m_EAX3HFReference;
  desc.LFReference = rec->m_EAX3LFReference;
  desc.RoomRolloffFactor = rec->m_EAX2RoomRolloff;
  desc.Diffusion = 0.0f;
  desc.Density = 0.0f;
  desc.Flags = 0;
  desc.ReflectionsPan[0] = 0.0f;
  desc.ReflectionsPan[1] = 0.0f;
  desc.ReflectionsPan[2] = 0.0f;
  desc.ReverbPan[0] = 0.0f;
  desc.ReverbPan[1] = 0.0f;
  desc.ReverbPan[2] = 0.0f;
}

void SndInterfaceSetProviderPrefs(unsigned int index, unsigned int indexUnderwater, unsigned int transitionDuration) {
  const SoundProviderPreferencesRec *rec;
  const SoundProviderPreferencesRec *recUnderwater;

  s_flags &= ~3;
  rec = g_soundProviderPreferencesDB.GetRecord(index);
  recUnderwater = g_soundProviderPreferencesDB.GetRecord(indexUnderwater);

  if (rec) {
    s_flags |= 1;
    SaveDesc(s_desc, rec);
  }

  if (recUnderwater) {
    s_flags |= 2;
    SaveDesc(s_descUnderwater, recUnderwater);
  }

  if ((g_underWater && !(s_flags & 2)) || (s_flags & 3) != 3) {
    const _FSOUND_REVERB_PROPERTIES blah = {
        0,     7.5f, 1.0f,  -10000, -10000, 0,       1.0f,   1.0f, 1.0f, -2602, 0.007f, {0.0f, 0.0f, 0.0f},
                                200, 0.011f, {0.0f, 0.0f, 0.0f},
        0.25f, 0.0f, 0.25f, 0.0f,   -5.0f,  5000.0f, 250.0f, 0.0f, 0.0f, 0.0f,  0x33F
    };

    s_currentProviderDesc = blah;
    StopProviderPrefFade();
    return;
  }

  const _FSOUND_REVERB_PROPERTIES &selected = g_underWater ? s_descUnderwater : s_desc;
  if (transitionDuration) {
    StartProviderPrefFade(selected, transitionDuration);
  } else {
    s_currentProviderDesc = selected;
    StopProviderPrefFade();
  }
}

void SndInterfaceSetProviderPrefs(const _FSOUND_REVERB_PROPERTIES &pref, const _FSOUND_REVERB_PROPERTIES &prefUnderwater) {
  s_desc = pref;
  s_descUnderwater = prefUnderwater;
  s_currentProviderDesc = g_underWater ? s_descUnderwater : s_desc;
  StopProviderPrefFade();
}

void SndInterfaceClearProviderPrefs(int indoors) {
  const SoundProviderPreferencesRec *rec = indoors ? ClientDBGetDefaultIndoorProviderPrefs() : ClientDBGetDefaultOutdoorProviderPrefs();

  if (rec) {
    SaveDesc(s_currentProviderDesc, rec);
    StopProviderPrefFade();
    return;
  }

  const _FSOUND_REVERB_PROPERTIES blah = {
      0,     7.5f, 1.0f,  -10000, -10000, 0,       1.0f,   1.0f, 1.0f, -2602, 0.007f, {0.0f, 0.0f, 0.0f},
                              200, 0.011f, {0.0f, 0.0f, 0.0f},
      0.25f, 0.0f, 0.25f, 0.0f,   -5.0f,  5000.0f, 250.0f, 0.0f, 0.0f, 0.0f,  0x33F
  };

  s_currentProviderDesc = blah;
  StopProviderPrefFade();
}

void SndSetRoomType(SNDROOMTYPE roomType) {
  s_startProviderDesc.Room = roomType;
  s_currentProviderDesc.Room = roomType;
  s_targetProviderDesc.Room = roomType;

  if (!s_idleRunning) {
    StopProviderPrefFade();
  }
}

static int EnvironmentHandler(const char *command, const char *arguments) {
  if (arguments && *arguments) {
    unsigned int index = SStrToUnsigned(arguments);

    SndInterfaceSetProviderPrefs(index, index, 0);
  }

  return 1;
}

static int EnvironmentListHandler(const char *command, const char *arguments) {
  int i = g_soundProviderPreferencesDB.GetNumRecords();

  while (i) {
    --i;
    const SoundProviderPreferencesRec *rec = g_soundProviderPreferencesDB.GetRecordByIndex(i);

    ASSERT(rec);
    ConsolePrintf("[%d]  %s", rec->m_ID, rec->m_Description);
  }

  return 1;
}

void ProviderPrefShutdown() {
  ConsoleCommandUnregister("env");
  ConsoleCommandUnregister("envlist");
}

void ProviderPrefInitialize() {
  s_flags = 0;
  ConsoleCommandRegister("env", EnvironmentHandler, DEBUG, "DEBUGGING");
  ConsoleCommandRegister("envlist", EnvironmentListHandler, DEBUG, "DEBUGGING");
}

void SndInterfaceProviderPrefsUnderwaterChanged() {
  const _FSOUND_REVERB_PROPERTIES *selected = 0;

  if (g_underWater) {
    if (s_flags & 2) {
      selected = &s_descUnderwater;
    }
  } else if (s_flags & 1) {
    selected = &s_desc;
  }

  if (selected) {
    s_currentProviderDesc = *selected;
  } else {
    const _FSOUND_REVERB_PROPERTIES blah = {
        0,     7.5f, 1.0f,  -10000, -10000, 0,       1.0f,   1.0f, 1.0f, -2602, 0.007f, {0.0f, 0.0f, 0.0f},
                                200, 0.011f, {0.0f, 0.0f, 0.0f},
        0.25f, 0.0f, 0.25f, 0.0f,   -5.0f,  5000.0f, 250.0f, 0.0f, 0.0f, 0.0f,  0x33F
    };

    s_currentProviderDesc = blah;
  }
  StopProviderPrefFade();
}
