#include <Base/Base.h>

#include "OsSound.h"

#include "Base/CDataAllocator.h"
#include "Event/EvtApi.h"
#include "Gx/Gx.h"
#include "Os/W32/OsISoundCache.h"
#include "Os/OsTime.h"
#include "Tempest/c34matrix.h"
#include "Tempest/cmath.h"

#include <new>

typedef signed char(__stdcall *FSOUND_STREAMCALLBACK)(FSOUND_STREAM *, LPVOID, int, int);
typedef LPVOID(__stdcall *FSOUND_ALLOCCALLBACK)(UINT);
typedef LPVOID(__stdcall *FSOUND_REALLOCCALLBACK)(LPVOID, UINT);
typedef void(__stdcall *FSOUND_FREECALLBACK)(LPVOID);
typedef UINT(__stdcall *FSOUND_OPENCALLBACK)(LPCSTR);
typedef void(__stdcall *FSOUND_CLOSECALLBACK)(UINT);
typedef int(__stdcall *FSOUND_READCALLBACK)(LPVOID, int, UINT);
typedef int(__stdcall *FSOUND_SEEKCALLBACK)(UINT, int, signed char);
typedef int(__stdcall *FSOUND_TELLCALLBACK)(UINT);

typedef bool (*SOUND_GET_PARAM_INT)(LPCSTR, int &);
typedef bool (*SOUND_GET_PARAM_FLOAT)(LPCSTR, float &);
typedef bool (*SOUND_GET_PARAM_STRING)(LPCSTR, LPCSTR &);

extern "C" void __stdcall        FSOUND_3D_SetDistanceFactor(float factor);
extern "C" void __stdcall        FSOUND_3D_SetDopplerFactor(float factor);
extern "C" void __stdcall        FSOUND_3D_SetRolloffFactor(float factor);
extern "C" float __stdcall       FSOUND_GetCPUUsage();
extern "C" signed char __stdcall FSOUND_3D_Listener_SetAttributes(
    const float *position,
    const float *velocity,
    float        forwardX,
    float        forwardY,
    float        forwardZ,
    float        topX,
    float        topY,
    float        topZ
);
extern "C" signed char __stdcall FSOUND_3D_SetAttributes(int channel, const float *position, const float *velocity);
extern "C" void __stdcall        FSOUND_3D_Listener_GetAttributes(
    float *position,
    float *velocity,
    float *forwardX,
    float *forwardY,
    float *forwardZ,
    float *topX,
    float *topY,
    float *topZ
);
extern "C" signed char __stdcall FSOUND_IsPlaying(int channel);
extern "C" void __stdcall        FSOUND_Close();
extern "C" void __stdcall        FSOUND_File_SetCallbacks(
    FSOUND_OPENCALLBACK  openCallback,
    FSOUND_CLOSECALLBACK closeCallback,
    FSOUND_READCALLBACK  readCallback,
    FSOUND_SEEKCALLBACK  seekCallback,
    FSOUND_TELLCALLBACK  tellCallback
);
extern "C" int __stdcall         FSOUND_GetDriver();
extern "C" signed char __stdcall FSOUND_GetDriverCaps(int driver, UINT *caps);
extern "C" LPCSTR __stdcall      FSOUND_GetDriverName(int driver);
extern "C" int __stdcall         FSOUND_GetError();
extern "C" int __stdcall         FSOUND_GetMaxChannels();
extern "C" int __stdcall         FSOUND_GetMixer();
extern "C" int __stdcall         FSOUND_GetNumDrivers();
extern "C" int __stdcall         FSOUND_GetNumHardwareChannels();
extern "C" int __stdcall         FSOUND_GetOutput();
extern "C" int __stdcall         FSOUND_GetOutputRate();
extern "C" signed char __stdcall FSOUND_Init(int mixRate, int maxSoftwareChannels, UINT flags);
extern "C" signed char __stdcall FSOUND_Sample_SetMinMaxDistance(FSOUND_SAMPLE *sample, float minDistance, float maxDistance);
extern "C" signed char __stdcall FSOUND_SetMute(int channel, signed char mute);
extern "C" signed char __stdcall FSOUND_SetPaused(int channel, signed char paused);
extern "C" signed char __stdcall FSOUND_SetFrequency(int channel, int frequency);
extern "C" signed char __stdcall FSOUND_SetBufferSize(int milliseconds);
extern "C" signed char __stdcall FSOUND_SetDriver(int driver);
extern "C" signed char __stdcall FSOUND_SetHWND(DWORD window);
extern "C" signed char __stdcall FSOUND_SetMaxHardwareChannels(int maximum);
extern "C" signed char __stdcall FSOUND_SetMemorySystem(
    LPVOID                 pool,
    int                    poolLength,
    FSOUND_ALLOCCALLBACK   allocCallback,
    FSOUND_REALLOCCALLBACK reallocCallback,
    FSOUND_FREECALLBACK    freeCallback
);
extern "C" signed char __stdcall    FSOUND_SetMinHardwareChannels(int minimum);
extern "C" signed char __stdcall    FSOUND_SetMixer(int mixer);
extern "C" signed char __stdcall    FSOUND_SetOutput(int output);
extern "C" signed char __stdcall    FSOUND_SetSFXMasterVolume(int volume);
extern "C" signed char __stdcall    FSOUND_SetVolume(int channel, int volume);
extern "C" signed char __stdcall    FSOUND_StopSound(int channel);
extern "C" signed char __stdcall    FSOUND_Stream_Close(FSOUND_STREAM *stream);
extern "C" FSOUND_STREAM *__stdcall FSOUND_Stream_Open(LPCSTR filename, UINT mode, int offset, int length);
extern "C" FSOUND_SAMPLE *__stdcall FSOUND_Stream_GetSample(FSOUND_STREAM *stream);
extern "C" int __stdcall            FSOUND_Stream_PlayEx(int channel, FSOUND_STREAM *stream, LPVOID dsp, signed char startPaused);
extern "C" signed char __stdcall    FSOUND_Stream_SetEndCallback(FSOUND_STREAM *stream, FSOUND_STREAMCALLBACK callback, int userdata);
extern "C" signed char __stdcall    FSOUND_Stream_SetLoopCount(FSOUND_STREAM *stream, int loopCount);
extern "C" int __stdcall            FSOUND_Stream_GetLengthMs(FSOUND_STREAM *stream);
extern "C" signed char __stdcall    FSOUND_Stream_SetTime(FSOUND_STREAM *stream, int milliseconds);
extern "C" signed char __stdcall    FSOUND_Stream_Stop(FSOUND_STREAM *stream);
extern "C" void __stdcall           FSOUND_Update();

static int    logFlags;
static int    s_maxCategorySounds[SOUNDCATEGORIES_NUMCATEGORIES] = {0x7FFFFFFF, 1, 2};
static LPCSTR s_outputSystemName[13] = {
    "No Sound", "Windows Mulimedia", "Direct Sound",      "A3D",      "Open Sound System",      "Enlightment Sound Daemon", "Alsa", "ASIO",
    "XBox",     "PlayStation 2",     "Mac Sound Manager", "Gamecube", "No Sound (non-realtime)"
};
static LPCSTR s_mixerName[10] = {
    "Low quality autodetect",
    "Blend mode (obsolete)",
    "MMXP5 (obsolete)",
    "MMXP6 (obsolete)",
    "Autodetect",
    "FPU",
    "MMXP5",
    "MMXP6",
    "Low quality mono",
    "Mono"
};
static bool  s_initialized;
static int   s_numChannels;
static int   s_numSoftwareChannels;
static int   s_num2dHardwareChannels;
static int   s_num3dHardwareChannels;
static int   s_mixRate = 44100;
static HSLOG s_log;
static LISTDECLEX(Sound, link, s_soundListActive);
static LISTDECLEX(Sound, fadeLink, s_soundListFade);
static LISTDECLEX(Sound, updateLink, s_soundListUpdate);
static LISTDECLEX(Sound, panningLink, s_soundListPanning);
static LISTDECLEX(Sound, cutoffLink, s_soundListCutoff);
static LISTDECLEX(Sound, stopLink, s_soundListStop);
static TInstanceAllocator<Sound> s_soundListFree(40);
static SCritSect                 s_soundSystemLock;
static bool                      s_globalPause;
static float                     s_soundVolume = 1.0f;
static float                     s_musicVolume = 0.5f;
static int                       s_muted = -1;
static long                      s_categoryCounts[SOUNDCATEGORIES_NUMCATEGORIES];
static int                       s_activeSoundCount;

struct InitParams {
  int   outputSystem;
  int   driver;
  int   mixer;
  int   bufferSize;
  int   minNumHardwareChannels;
  int   maxNumHardwareChannels;
  int   mixRate;
  int   numSoftwareChannels;
  int   flags;
  float distanceFactor;
  float dopplerFactor;
  float rolloffFactor;
  int   cacheSizeMB;
};

BYTE (*Sound::m_positionUpdateCallback)(LONGLONG, NTempest::C3Vector &);

static signed char __stdcall FSoundStreamEndCallback(FSOUND_STREAM *stream, LPVOID buff, int len, int param);
static LPVOID __stdcall      FSoundAllocCallback(UINT size);
static LPVOID __stdcall      FSoundReallocCallback(LPVOID ptr, UINT size);
static void __stdcall        FSoundFreeCallback(LPVOID ptr);
static BOOL                  SoundIdle(LPCVOID, LPVOID);
static int                   CheckInitError(char success, LPCSTR function, int parameter);
static void
InitializeParams(InitParams &params, SOUND_GET_PARAM_INT GetParamInt, SOUND_GET_PARAM_FLOAT GetParamFloat, SOUND_GET_PARAM_STRING GetParamString);

static LPVOID __stdcall FSoundAllocCallback(UINT size) {
  return SMemAlloc(size, "FMod", 0, 0);
}

static LPVOID __stdcall FSoundReallocCallback(LPVOID ptr, UINT size) {
  return SMemReAlloc(ptr, size, "FMod", 0, 0);
}

static void __stdcall FSoundFreeCallback(LPVOID ptr) {
  SMemFree(ptr, "FMod", 0, 0);
}

static signed char __stdcall FSoundStreamEndCallback(FSOUND_STREAM *stream, LPVOID buff, int len, int param) {
  ASSERT(param);

  s_soundSystemLock.Enter();
  s_soundListStop.LinkNode(reinterpret_cast<Sound *>(param), LIST_TAIL, 0);
  s_soundSystemLock.Leave();

  return 0;
}

static BOOL SoundIdle(LPCVOID, LPVOID) {
  if (!s_globalPause) {
    Sound::Update();
  }

  return 1;
}

Sound::Sound()
    : m_channel(-1),
      m_stream(0),
      m_flags(0),
      m_worldPosition(),
      m_velocity(),
      m_updateHandle(0),
      m_volume(1.0f),
      m_freq(22050),
      m_fileNameHashed(-1),
      m_category(SOUNDCATEGORY_NONE) {
  ++s_activeSoundCount;
}

Sound::~Sound() {
  Stop();
  DecrementCategory(m_category);
  --s_activeSoundCount;
}

static int CheckInitError(char success, LPCSTR function, int parameter) {
  if (!success) {
    int error = FSOUND_GetError();
    SLogWrite(s_log, "Error: %s(%i) returned %i", function, parameter, error);
    return error;
  }

  return 0;
}

static void
InitializeParams(InitParams &params, SOUND_GET_PARAM_INT GetParamInt, SOUND_GET_PARAM_FLOAT GetParamFloat, SOUND_GET_PARAM_STRING GetParamString) {
  params.outputSystem = -1;
  params.driver = -1;
  params.mixer = -1;
  params.bufferSize = 0;
  params.minNumHardwareChannels = -1;
  params.maxNumHardwareChannels = -1;
  params.mixRate = 44100;
  params.numSoftwareChannels = 12;
  params.flags = 0x80;
  params.distanceFactor = 1.0f;
  params.dopplerFactor = 1.0f;
  params.rolloffFactor = 1.0f;
  params.cacheSizeMB = 1;

  GetParamInt("SoundOutputSystem", params.outputSystem);
  GetParamInt("SoundDriver", params.driver);
  GetParamInt("SoundMixer", params.mixer);
  GetParamInt("SoundBufferSize", params.bufferSize);
  GetParamInt("SoundMinHardwareChannels", params.minNumHardwareChannels);
  GetParamInt("SoundMaxHardwareChannels", params.maxNumHardwareChannels);
  GetParamInt("SoundMixRate", params.mixRate);
  GetParamInt("SoundSoftwareChannels", params.numSoftwareChannels);
  GetParamInt("SoundInitFlags", params.flags);
  GetParamFloat("SoundDistanceFactor", params.distanceFactor);
  GetParamFloat("SoundDopplerFactor", params.dopplerFactor);
  GetParamFloat("SoundRolloffFactor", params.rolloffFactor);
  GetParamInt("SoundMemoryCache", params.cacheSizeMB);

  params.flags &= ~0x200;
}

int Sound::Initialize(bool (*GetParamInt)(LPCSTR, int &), bool (*GetParamFloat)(LPCSTR, float &), bool (*GetParamString)(LPCSTR, LPCSTR &)) {
  int        error = 0;
  InitParams params;
  UINT       caps;
  int        parameter;
  int        driver;
  int        mixer;
  int        output;
  int        numHardwareChannels;

  ASSERT(GetParamInt);
  ASSERT(GetParamFloat);
  ASSERT(GetParamString);

  SLogCreate("Sound.log", logFlags, &s_log);
  logFlags |= 4;
  SLogWrite(s_log, "Sound::Initialize()");

  if (s_initialized) {
    SLogWrite(s_log, "Already Initialized");
    goto done;
  }

  InitializeParams(params, GetParamInt, GetParamFloat, GetParamString);

  parameter = params.outputSystem;
  if (parameter < -1 || parameter > 12) {
    error = 14;
    goto done;
  }
  error = CheckInitError(FSOUND_SetOutput(parameter) != 0, "FSOUND_SetOutput", parameter);
  if (error) {
    goto done;
  }

  driver = params.driver;
  if (driver < -1 || driver >= FSOUND_GetNumDrivers()) {
    error = 14;
    goto done;
  }
  if (driver == -1) {
    driver = 0;
  }
  parameter = driver;
  error = CheckInitError(FSOUND_SetDriver(parameter) != 0, "FSOUND_SetDriver", parameter);
  if (error) {
    goto done;
  }
  FSOUND_GetDriverCaps(driver, &caps);

  mixer = params.mixer;
  if (mixer < -1 || mixer >= 10) {
    error = 14;
    goto done;
  }
  if (mixer == -1) {
    mixer = 4;
  }
  parameter = mixer;
  error = CheckInitError(FSOUND_SetMixer(parameter) != 0, "FSOUND_SetMixer", parameter);
  if (error) {
    goto done;
  }

  parameter = params.bufferSize;
  if (parameter < 0) {
    error = 14;
    goto done;
  }
  if (parameter) {
    error = CheckInitError(FSOUND_SetBufferSize(parameter) != 0, "FSOUND_SetBufferSize", parameter);
    if (error) {
      goto done;
    }
  }

  parameter = GxDevWindow();
  error = CheckInitError(FSOUND_SetHWND(parameter) != 0, "FSOUND_SetHWND", parameter);
  if (error) {
    goto done;
  }

  parameter = params.minNumHardwareChannels;
  if (parameter < -1) {
    error = 14;
    goto done;
  }
  if (parameter != -1) {
    error = CheckInitError(FSOUND_SetMinHardwareChannels(parameter) != 0, "FSOUND_SetMinHardwareChannels", parameter);
    if (error) {
      goto done;
    }
  }

  parameter = params.maxNumHardwareChannels;
  if (parameter < -1) {
    error = 14;
    goto done;
  }
  if (parameter != -1) {
    error = CheckInitError(FSOUND_SetMaxHardwareChannels(parameter) != 0, "FSOUND_SetMaxHardwareChannels", parameter);
    if (error) {
      goto done;
    }
  }

  if (!FSOUND_SetMemorySystem(0, 0, FSoundAllocCallback, FSoundReallocCallback, FSoundFreeCallback)) {
    error = FSOUND_GetError();
    SLogWrite(s_log, "Error: FSOUND_SetMemorySystem(NULL, 0, <SMem wrappers>) returned %i", error);
    goto done;
  }
  SLogWrite(s_log, "memory system configured: SMem wrappers");

  SoundFileCache::Initialize(params.cacheSizeMB);
  FSOUND_File_SetCallbacks(SoundFileCache::Open, SoundFileCache::Close, SoundFileCache::Read, SoundFileCache::Seek, SoundFileCache::Tell);
  FSOUND_3D_SetDistanceFactor(params.distanceFactor);
  FSOUND_3D_SetDopplerFactor(params.dopplerFactor);
  FSOUND_3D_SetRolloffFactor(params.rolloffFactor);

  s_numSoftwareChannels = params.numSoftwareChannels;
  if (!FSOUND_Init(params.mixRate, params.numSoftwareChannels, params.flags)) {
    error = FSOUND_GetError();
    SLogWrite(s_log, "Error: FSOUND_Init(%i, %i, %i) returned %i", params.mixRate, params.numSoftwareChannels, params.flags, error);
    goto done;
  }

  s_mixRate = params.mixRate;
  output = FSOUND_GetOutput();
  SLogWrite(s_log, "Output system: %i '%s'", parameter, s_outputSystemName[output]);

  driver = FSOUND_GetDriver();
  SLogWrite(s_log, "Driver: %i '%s' %08x", driver, FSOUND_GetDriverName(driver), caps);

  mixer = FSOUND_GetMixer();
  SLogWrite(s_log, "Mixer: %i '%s'", driver, s_mixerName[mixer]);

  if (params.bufferSize) {
    SLogWrite(s_log, "Buffer size: %ims", params.bufferSize);
  } else {
    SLogWrite(s_log, "Buffer size: default");
  }

  s_numChannels = FSOUND_GetMaxChannels();
  numHardwareChannels = FSOUND_GetNumHardwareChannels();
  s_num2dHardwareChannels = s_numChannels - numHardwareChannels - s_numSoftwareChannels;
  s_num3dHardwareChannels = numHardwareChannels;
  SLogWrite(
      s_log, "Channels: %i (%i software, %i 2D hardware(dsound), %i 3D hardware)", s_numChannels, s_numSoftwareChannels, s_num2dHardwareChannels,
      s_num3dHardwareChannels
  );
  SLogWrite(s_log, "Output rate: %i Hz", FSOUND_GetOutputRate());
  SLogWrite(s_log, "Initialization flags: %08x", params.flags);

  EventRegister(EVENT_ID_IDLE, SoundIdle);
  SLogWrite(s_log, "Sound::Initialize() complete", 3.71f);

  s_categoryCounts[0] = 0;
  s_categoryCounts[1] = 0;
  s_categoryCounts[2] = 0;
  s_globalPause = false;
  s_initialized = true;

done:
  SLogFlush(s_log);
  return error;
}

void Sound::Shutdown() {
  Sound *sound;

  sound = s_soundListActive.Head();
  while (sound) {
    s_soundListFree.Put(sound);
    sound = s_soundListActive.Head();
  }

  if (s_initialized) {
    EventUnregister(EVENT_ID_IDLE, SoundIdle);
    FSOUND_StopSound(-3);
    FSOUND_Close();
    SoundFileCache::Shutdown();
    SLogFlush(s_log);
    s_initialized = false;
    s_muted = -1;
  }
}

void Sound::Update() {
  NTempest::C3Vector pos;
  NTempest::C3Vector listenerPos;

  ProcessStopList();
  ProcessFadeList();
  ProcessUpdateList();

  FSOUND_3D_Listener_GetAttributes(&pos.x, 0, 0, 0, 0, 0, 0, 0);
  listenerPos.x = pos.z;
  listenerPos.y = -pos.x;
  listenerPos.z = pos.y;

  ProcessPanningList(listenerPos);
  ProcessCutoffList(listenerPos);
  FSOUND_Update();
}

void Sound::ProcessStopList() {
  s_soundSystemLock.Enter();

  while (Sound *sound = s_soundListStop.Head()) {
    s_soundListStop.UnlinkNode(sound);
    s_soundSystemLock.Leave();

    if (sound->m_flags & 0x00000004) {
      sound->Stop();
    } else {
      s_soundListFree.Put(sound);
    }

    s_soundSystemLock.Enter();
  }

  s_soundSystemLock.Leave();
}

void Sound::ProcessFadeList() {
  UINT   timestamp = OsGetAsyncTimeMs();
  Sound *sound;
  Sound *next;

  for (sound = s_soundListFade.Head(); sound; sound = next) {
    next = s_soundListFade.Next(sound);

    if (sound->m_channel == -1) {
      continue;
    }

    ASSERT(sound->m_stream);

    int volume = static_cast<int>(static_cast<double>(timestamp - sound->m_fadeStartTime) * sound->m_fadeRate);

    if (sound->m_fadeRate < 0.0f) {
      volume += sound->m_fadeVolume;
      if (volume <= 0) {
        sound->RemoveFromFadeList();

        if (sound->m_flags & 0x01000000) {
          sound->RemoveFromFadeList();
          sound->Suspend();
        } else if (sound->m_flags & 0x00000004) {
          sound->Stop();
        } else {
          s_soundListFree.Put(sound);
        }

        continue;
      }
    } else if (volume >= sound->m_fadeVolume) {
      volume = sound->m_fadeVolume;
      sound->RemoveFromFadeList();
    }

    sound->SetVolume(volume);
  }
}

void Sound::ProcessUpdateList() {
  NTempest::C3Vector soundPosition;
  NTempest::C3Vector worldPosition;

  if (!m_positionUpdateCallback) {
    return;
  }

  ITERATELIST(Sound, s_soundListUpdate, sound) {
    if (sound->m_channel == -1) {
      continue;
    }

    if (m_positionUpdateCallback(sound->m_updateHandle, worldPosition)) {
      soundPosition.x = -worldPosition.y;
      soundPosition.y = worldPosition.z;
      soundPosition.z = worldPosition.x;

      sound->m_worldPosition = worldPosition;
      FSOUND_3D_SetAttributes(sound->m_channel, &soundPosition.x, 0);
    }
  }
}

void Sound::ProcessPanningList(const NTempest::C3Vector &listenerPos) {
  NTempest::C34Matrix rotate;
  NTempest::C3Vector  soundVirtualPosition;
  NTempest::C3Vector  cross;
  NTempest::C3Vector  offset;
  float               rotationAngle;

  ITERATELIST(Sound, s_soundListPanning, sound) {
    if (sound->m_channel == -1) {
      continue;
    }

    rotationAngle = min(max(sound->m_panning, 0.0f), 1.0f) * 1.5707964f;
    offset.x = sound->m_worldPosition.x - listenerPos.x;
    offset.y = sound->m_worldPosition.y - listenerPos.y;
    offset.z = 0.0f;

    if (NTempest::CMath::fabs_(offset.x) < 0.001f && NTempest::CMath::fabs_(offset.y) < 0.001f) {
      continue;
    }

    cross = NTempest::C3Vector::Cross(NTempest::C3Vector(0.0f, 0.0f, 1.0f), offset);
    rotate.Rotate(rotationAngle, cross, false);
    offset = offset * rotate;
    offset.x += listenerPos.x;
    offset.y += listenerPos.y;
    offset.z += listenerPos.z;

    soundVirtualPosition.x = -offset.y;
    soundVirtualPosition.y = offset.z;
    soundVirtualPosition.z = offset.x;
    FSOUND_3D_SetAttributes(sound->m_channel, &soundVirtualPosition.x, 0);
  }
}

void Sound::ProcessCutoffList(const NTempest::C3Vector &listenerPos) {
  ITERATELIST(Sound, s_soundListCutoff, sound) {
    NTempest::C3Vector distance = sound->m_worldPosition - listenerPos;

    if (distance.SquaredMag() <= sound->m_cutoffDistanceSquared) {
      if (sound->m_flags & 0x01000000) {
        sound->m_flags &= ~0x01000000U;
        sound->Resume();

        if (sound->m_flags & 0x80000000) {
          sound->SetFadeIn(2.0f, sound->m_fadeVolume / 255.0f);
        } else {
          sound->SetFadeIn(2.0f, sound->m_volume);
        }
      }
    } else if (!(sound->m_flags & 0x01000000)) {
      if (sound->m_flags & 0x04000000) {
        sound->m_flags |= 0x01000000;
      }
      sound->Stop(2.0f);
    }
  }
}

Sound *Sound::Alloc(LPCSTR name) {
  Sound *sound = s_soundListFree.Get(0);

  s_soundListActive.LinkNode(sound, LIST_TAIL, 0);
  if (name) {
    sound->m_fileNameHashed = SStrHash(name, 0, 0);
  }

  return sound;
}

Sound *Sound::Play(SOUNDCATEGORIES category, LPCSTR filename, UINT mode, bool startPaused, int flags) {
  Sound *sound = Alloc(filename);
  ASSERT(sound);

  sound->m_flags |= flags & 0xFF;
  if (!(sound->m_flags & 0x00000002) && s_muted > 0) {
    return 0;
  }

  ASSERT(filename);
  sound->m_stream = FSOUND_Stream_Open(filename, mode, 0, 0);
  if (!sound->m_stream) {
    s_soundListFree.Put(sound);
    return 0;
  }

  FSOUND_Stream_SetEndCallback(sound->m_stream, FSoundStreamEndCallback, reinterpret_cast<int>(sound));
  sound->m_category = category;

  if (!startPaused) {
    sound->m_channel = FSOUND_Stream_PlayEx(-1, sound->m_stream, 0, 0);
    if (sound->m_channel == -1) {
      s_soundListFree.Put(sound);
      return 0;
    }

    sound->IncrementCategory(sound->m_category);
    sound->UpdateVolume();
    sound->UpdatePosition();
  }

  return sound;
}

Sound *Sound::Play2D(SOUNDCATEGORIES category, LPCSTR filename, int flags, bool startPaused) {
  if (DupeCheckFailed(category, filename, flags)) {
    return 0;
  }

  return Play(category, filename, 0x2000, startPaused, flags);
}

Sound *Sound::Play3D(SOUNDCATEGORIES category, LPCSTR filename, int flags, bool startPaused) {
  if (DupeCheckFailed(category, filename, flags)) {
    return 0;
  }

  Sound *sound = Play(category, filename, 0x1000, startPaused, flags);
  if (!sound) {
    return 0;
  }

  sound->m_flags |= 0x08000000;
  if (!startPaused) {
    sound->SetPaused(false);
  }

  return sound;
}

Sound *Sound::PlayLooped(SOUNDCATEGORIES category, LPCSTR filename, int loopCount, UINT mode, bool startPaused, int flags) {
  ASSERT(loopCount >= -1);

  Sound *sound = Alloc(filename);
  ASSERT(sound);

  sound->m_flags |= flags & 0xFF;
  ASSERT(filename);

  sound->m_stream = FSOUND_Stream_Open(filename, mode, 0, 0);
  if (!sound->m_stream) {
    s_soundListFree.Put(sound);
    return 0;
  }

  if (loopCount) {
    FSOUND_Stream_SetLoopCount(sound->m_stream, loopCount);
    FSOUND_Stream_SetEndCallback(sound->m_stream, FSoundStreamEndCallback, reinterpret_cast<int>(sound));
  }

  sound->m_category = category;
  if (!startPaused) {
    sound->m_channel = FSOUND_Stream_PlayEx(-1, sound->m_stream, 0, 0);
    if (sound->m_channel == -1) {
      s_soundListFree.Put(sound);
      return 0;
    }

    if (s_muted > 0) {
      FSOUND_SetMute(sound->m_channel, 1);
    } else {
      sound->UpdateVolume();
    }

    sound->UpdatePosition();
    sound->IncrementCategory(category);
  }

  sound->m_flags |= 0x04000000;
  return sound;
}

Sound *Sound::Play2DLooped(SOUNDCATEGORIES category, LPCSTR filename, int flags, UINT loopCount, bool startPaused) {
  if (DupeCheckFailed(category, filename, flags)) {
    return 0;
  }

  if (loopCount == 1) {
    return Play2D(category, filename, flags, startPaused);
  }

  return PlayLooped(category, filename, static_cast<int>(loopCount - 1), 0x2002, startPaused, flags);
}

Sound *Sound::Play3DLooped(SOUNDCATEGORIES category, LPCSTR filename, int flags, UINT loopCount, bool startPaused) {
  Sound *sound;

  if (DupeCheckFailed(category, filename, flags)) {
    return 0;
  }

  if (loopCount == 1) {
    return Play3D(category, filename, flags, startPaused);
  }

  sound = PlayLooped(category, filename, static_cast<int>(loopCount - 1), 0x1002, startPaused, flags);
  if (sound) {
    if (!startPaused) {
      sound->SetPaused(false);
    }
    sound->m_flags |= 0x08000000;
  }

  return sound;
}

void Sound::KillSound(Sound *&sound) {
  if (sound) {
    Sound *released = sound;
    s_soundListFree.Put(released);
  }

  sound = 0;
}

void Sound::SetFadeIn(float fadeTime, float volume) {
  ASSERT(volume >= 0.0f && volume <= 1.0f);

  if (m_stream && fadeTime >= 0.1f) {
    m_fadeVolume = static_cast<int>(volume * 255.0f);
    m_fadeRate = m_fadeVolume / (fadeTime * 1000.0f);

    if (m_channel != -1) {
      m_fadeStartTime = OsGetAsyncTimeMs();
      AddToFadeList();
    }
  }
}

void Sound::SetFadeIn(UINT fadeTime, float volume) {
  ASSERT(volume >= 0.0f && volume <= 1.0f);

  if (m_stream && fadeTime) {
    m_fadeVolume = static_cast<int>(volume * 255.0f);
    m_fadeRate = m_fadeVolume / static_cast<float>(fadeTime);

    if (!IsSuspended()) {
      ASSERT(m_channel == -1);
      m_fadeStartTime = OsGetAsyncTimeMs();
      AddToFadeList();
    }
  }
}

void Sound::Set3DUpdateHandle(LONGLONG handle) {
  if (m_channel == -1 || !m_stream || !(m_flags & 0x08000000)) {
    return;
  }

  m_updateHandle = handle;
  if (handle) {
    AddToUpdateList();
  } else {
    RemoveFromUpdateList();
  }
}

bool Sound::IsPlaying() {
  if (IsSuspended()) {
    return true;
  }
  if (m_channel == -1) {
    return false;
  }
  return FSOUND_IsPlaying(m_channel) != 0;
}

bool Sound::IsStopping() {
  if (!(m_flags & 0x80000000)) {
    return false;
  }
  return m_fadeRate <= 0.0f;
}

bool Sound::IsOutOfRange() {
  return (m_flags & 0x01000000) != 0;
}

void Sound::Suspend() {
  if (IsSuspended()) {
    return;
  }

  if (m_stream) {
    FSOUND_Stream_SetEndCallback(m_stream, 0, 0);
  }

  DecrementCategory(m_category);

  if (m_channel != -1) {
    if (FSOUND_IsPlaying(m_channel)) {
      FSOUND_StopSound(m_channel);
    }
    m_channel = -1;
  }

  if (m_stream) {
    FSOUND_Stream_Stop(m_stream);
  }

  m_suspendedFlags = m_flags;
  m_flags |= 0x00800000;
  RemoveFromUpdateList();
  RemoveFromPanningList();
}

void Sound::Resume() {
  if (!IsSuspended()) {
    return;
  }

  FSOUND_Stream_SetEndCallback(m_stream, FSoundStreamEndCallback, reinterpret_cast<int>(this));
  m_channel = FSOUND_Stream_PlayEx(-1, m_stream, 0, 1);
  if (m_channel == -1) {
    Stop();
    return;
  }

  IncrementCategory(m_category);
  if (m_suspendedFlags & 0x40000000) {
    AddToUpdateList();
  }
  if (m_suspendedFlags & 0x20000000) {
    AddToPanningList();
  }

  m_flags &= ~0x00800000U;
  UpdatePosition();
  UpdateVolume();
  if (m_flags & 0x02000000) {
    SetFrequency(m_freq);
  }
  FSOUND_SetPaused(m_channel, 0);
}

void Sound::Stop() {
  if (m_stream) {
    FSOUND_Stream_SetEndCallback(m_stream, 0, 0);
  }

  DecrementCategory(m_category);

  if (m_channel != -1) {
    if (FSOUND_IsPlaying(m_channel)) {
      FSOUND_StopSound(m_channel);
    }
    m_channel = -1;
  }

  if (m_stream) {
    FSOUND_Stream_Stop(m_stream);
    FSOUND_Stream_Close(m_stream);
    m_stream = 0;
  }

  RemoveFromFadeList();
  RemoveFromUpdateList();
  RemoveFromPanningList();
  RemoveFromCutoffList();
}

void Sound::UpdateVolume() {
  if (m_channel != -1 && (m_flags & 0x00080000)) {
    SetVolume(m_volume);
  }
}

void Sound::UpdatePosition() {
  if ((m_flags & 0x08000000) && (m_flags & 0x00040000)) {
    SetPosition(m_worldPosition, (m_flags & 0x00400000) ? &m_velocity : 0);
  }
}

void Sound::Stop(float fadeTime) {
  if (m_channel != -1 && m_stream && fadeTime >= 0.1f) {
    m_fadeVolume = GetVolume();
    m_fadeRate = m_fadeVolume / (fadeTime * -1000.0f);
    m_fadeStartTime = OsGetAsyncTimeMs();
    AddToFadeList();
  } else {
    Stop();
  }
}

void Sound::Stop(UINT fadeTime) {
  if (m_channel != -1 && m_stream && fadeTime >= 100) {
    m_fadeVolume = GetVolume();
    m_fadeRate = m_fadeVolume / -static_cast<float>(fadeTime);
    m_fadeStartTime = OsGetAsyncTimeMs();
    AddToFadeList();
  } else {
    Stop();
  }
}

bool Sound::SetPaused(bool state) {
  if (!m_stream || IsSuspended()) {
    return false;
  }

  if (m_channel != -1) {
    FSOUND_SetPaused(m_channel, state);
    return true;
  }

  if (state) {
    return true;
  }

  m_channel = FSOUND_Stream_PlayEx(-1, m_stream, 0, 1);
  if (m_channel == -1) {
    return false;
  }

  if (s_muted > 0) {
    FSOUND_SetMute(m_channel, 1);
  }

  IncrementCategory(m_category);
  UpdateVolume();
  UpdatePosition();
  FSOUND_SetPaused(m_channel, 0);
  return true;
}

int Sound::GetLengthMs() {
  ASSERT(m_stream);
  return FSOUND_Stream_GetLengthMs(m_stream);
}

int Sound::SetPositionMs(int milliseconds) {
  ASSERT(m_stream);
  return FSOUND_Stream_SetTime(m_stream, milliseconds);
}

void Sound::SetPosition(const NTempest::C3Vector &worldPosition, const NTempest::C3Vector *vel) {
  NTempest::C3Vector soundPosition(-worldPosition.y, worldPosition.z, worldPosition.x);
  NTempest::C3Vector velocity;

  if (!(m_flags & 0x08000000)) {
    return;
  }

  m_flags |= 0x00040000;
  m_worldPosition = worldPosition;

  if (vel) {
    m_flags |= 0x00400000;
    m_velocity = *vel;
  }

  if (IsSuspended() || m_channel == -1) {
    return;
  }

  if (vel) {
    velocity.x = -vel->y;
    velocity.y = vel->z;
    velocity.z = vel->x;
  }

  FSOUND_3D_SetAttributes(m_channel, &soundPosition.x, vel ? &velocity.x : 0);
}

void Sound::SetReverbProperties(const _FSOUND_REVERB_CHANNELPROPERTIES *reverb) {
}

void Sound::SetPanning(float pan) {
  if (!(m_flags & 0x08000000)) {
    return;
  }

  if (pan == -1.0f) {
    RemoveFromPanningList();
    return;
  }

  m_panning = min(max(pan, 0.0f), 1.0f);
  if (m_channel != -1 && !(m_flags & 0x20000000)) {
    AddToPanningList();
  }
}

void Sound::SetCutoffDistanceSquared(float distanceSquared) {
  if (!(m_flags & 0x08000000)) {
    return;
  }

  m_cutoffDistanceSquared = distanceSquared;
  if (distanceSquared == 0.0f) {
    RemoveFromCutoffList();
  } else {
    AddToCutoffList();
  }
}

int Sound::GetVolume() {
  if (m_channel != -1) {
    return NTempest::CMath::ftol_round_0_256_(m_volume * 255.0f);
  }

  return 0;
}

void Sound::SetVolume(int volume) {
  ASSERT(volume >= 0 && volume <= 255);
  SetVolume(volume / 255.0f);
}

void Sound::SetVolume(float volume) {
  ASSERT(volume >= 0.0f && volume <= 1.0f);

  m_flags |= 0x00080000;
  m_volume = volume;

  if (!IsSuspended() && m_channel != -1) {
    float categoryVolume = (m_flags & 0x00000002) ? s_musicVolume : s_soundVolume;

    FSOUND_SetVolume(m_channel, static_cast<int>(categoryVolume * volume * 255.0f));
  }
}

void Sound::SetFrequency(int freq) {
  m_flags |= 0x02000000;
  m_freq = freq;

  if (!IsSuspended() && m_channel != -1) {
    FSOUND_SetFrequency(m_channel, m_freq);
  }
}

void Sound::SetDistances(float min, float max) {
  if (m_stream && (m_flags & 0x08000000)) {
    FSOUND_SAMPLE *sample = FSOUND_Stream_GetSample(m_stream);
    if (sample) {
      FSOUND_Sample_SetMinMaxDistance(sample, min, max);
    }
  }
}

void Sound::SetListenerAttributes(
    const NTempest::C3Vector &worldPosition,
    const NTempest::C3Vector *worldVelocity,
    const NTempest::C3Vector &worldForward,
    const NTempest::C3Vector &worldUp
) {
  NTempest::C3Vector soundPosition(-worldPosition.y, worldPosition.z, worldPosition.x);
  NTempest::C3Vector soundForward(-worldForward.y, worldForward.z, worldForward.x);
  NTempest::C3Vector soundUp(-worldUp.y, worldUp.z, worldUp.x);
  NTempest::C3Vector soundVelocity;
  if (worldVelocity) {
    soundVelocity.x = -worldVelocity->y;
    soundVelocity.y = worldVelocity->z;
    soundVelocity.z = worldVelocity->x;
  }
  FSOUND_3D_Listener_SetAttributes(
      &soundPosition.x, worldVelocity ? &soundVelocity.x : 0, soundForward.x, soundForward.y, soundForward.z, soundUp.x, soundUp.y, soundUp.z
  );
}

void Sound::GetListenerPosition(NTempest::C3Vector &position) {
  NTempest::C3Vector soundPos;

  FSOUND_3D_Listener_GetAttributes(&soundPos.x, 0, 0, 0, 0, 0, 0, 0);
  position.x = soundPos.z;
  position.y = -soundPos.x;
  position.z = soundPos.y;
}

void Sound::SetReverbProperties(const _FSOUND_REVERB_PROPERTIES *reverb) {
}

void Sound::AddToFadeList() {
  if (!(m_flags & 0x80000000)) {
    s_soundListFade.LinkNode(this, LIST_TAIL, 0);
    m_flags |= 0x80000000;
  }
}

void Sound::RemoveFromFadeList() {
  if (m_flags & 0x80000000) {
    SetVolume(m_fadeVolume);
    fadeLink.Unlink();
    m_flags &= ~0x80000000;
  }
}

void Sound::AddToUpdateList() {
  if (!(m_flags & 0x40000000)) {
    s_soundListUpdate.LinkNode(this, LIST_TAIL, 0);
    m_flags |= 0x40000000;
  }
}

void Sound::RemoveFromUpdateList() {
  if (m_flags & 0x40000000) {
    updateLink.Unlink();
    m_flags &= ~0x40000000;
  }
}

void Sound::AddToPanningList() {
  if (!(m_flags & 0x20000000)) {
    s_soundListPanning.LinkNode(this, LIST_TAIL, 0);
    m_flags |= 0x20000000;
  }
}

void Sound::RemoveFromPanningList() {
  if (m_flags & 0x20000000) {
    panningLink.Unlink();
    m_flags &= ~0x20000000;
  }
}

void Sound::AddToCutoffList() {
  if (!(m_flags & 0x10000000)) {
    s_soundListCutoff.LinkNode(this, LIST_TAIL, 0);
    m_flags |= 0x10000000;
  }
}

void Sound::RemoveFromCutoffList() {
  if (m_flags & 0x10000000) {
    cutoffLink.Unlink();
    m_flags &= ~0x10000000;
  }
}

UINT SndGetCPUPerformance() {
  return static_cast<UINT>(FSOUND_GetCPUUsage());
}

int Sound::GetNumOutputSystems() {
  return 13;
}

LPCSTR Sound::GetOutputSystemName(int index) {
  ASSERT(index >= 0 && index <= 12);
  return s_outputSystemName[index];
}

int Sound::GetNumDrivers() {
  return FSOUND_GetNumDrivers();
}

LPCSTR Sound::GetDriverName(int index) {
  ASSERT(index >= 0 && index < FSOUND_GetNumDrivers());
  return FSOUND_GetDriverName(index);
}

int Sound::GetNumMixers() {
  return 10;
}

LPCSTR Sound::GetMixerName(int index) {
  ASSERT(index >= 0 && index < 10);
  return s_mixerName[index];
}

void Sound::SetSoundVolume(float volume) {
  volume = min(max(0.0f, volume), 1.0f);
  s_soundVolume = volume;
  UpdateSoundVolumes(false);
}

void Sound::SetMusicVolume(float volume) {
  volume = min(max(0.0f, volume), 1.0f);
  s_musicVolume = volume;
  UpdateSoundVolumes(true);
}

void Sound::SetMasterVolume(float volume) {
  volume = min(max(0.0f, volume), 1.0f);
  FSOUND_SetSFXMasterVolume(NTempest::CMath::ftol_round_0_256_(volume * 255.0f));
}

void Sound::MuteSFX(bool m) {
  int    muted = m != false;
  Sound *sound;
  Sound *next;

  if (muted != s_muted) {
    s_muted = muted;
    for (sound = s_soundListActive.Head(); sound; sound = next) {
      next = sound->link.Next();
      if (sound->m_channel != -1 && !(sound->m_flags & 0x00000002)) {
        FSOUND_SetMute(sound->m_channel, s_muted);
      }
    }
  }
}

void Sound::UpdateSoundVolumes(bool music) {
  Sound *sound;
  Sound *next;

  for (sound = s_soundListActive.Head(); sound; sound = next) {
    next = sound->link.Next();
    if (sound->m_channel != -1 && ((sound->m_flags & 0x00000002) != 0) == music) {
      sound->UpdateVolume();
    }
  }
}

bool Sound::DupeCheckFailed(SOUNDCATEGORIES category, LPCSTR fileName, int flags) {
  if (category >= SOUNDCATEGORIES_NUMCATEGORIES) {
    return true;
  }

  if (s_categoryCounts[category] >= s_maxCategorySounds[category]) {
    return true;
  }

  if (flags & 0x1) {
    UINT filenameHash = SStrHash(fileName, 0, 0);

    ITERATELIST(Sound, s_soundListActive, sound) {
      if (sound->m_fileNameHashed == filenameHash && (!(sound->m_flags & 0x80000000) || (sound->m_flags & 0x01000000))) {
        return true;
      }
    }
  }

  return false;
}

bool Sound::IsSuspended() const {
  return (m_flags & 0x00800000) != 0;
}

void Sound::IncrementCategory(SOUNDCATEGORIES category) {
  if (!(m_flags & 0x00100000)) {
    m_flags |= 0x00100000;
    SInterlockedIncrement(&s_categoryCounts[category]);
  }
}

void Sound::DecrementCategory(SOUNDCATEGORIES category) {
  if (m_flags & 0x00100000) {
    m_flags &= ~0x00100000;
    if (s_categoryCounts[category]) {
      SInterlockedDecrement(&s_categoryCounts[category]);
    }
  }
}

int Sound::GetMixRate() {
  return s_mixRate;
}
