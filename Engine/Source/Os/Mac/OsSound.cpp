#include <Base/Base.h>

#include "Os/W32/OsSound.h"

#include "Event/EvtApi.h"
#include "Os/Mac/GlobalAUGraph.h"
#include "Os/Mac/LoaderThread.h"
#include "Os/Mac/MIDIPlayer.h"
#include "Os/Mac/MP3Player.h"
#include "Os/Mac/Recycler.h"
#include "Os/Mac/StreamingCache.h"
#include "Os/Mac/WAVPlayer.h"
#include "Os/OsTime.h"
#include "Tempest/c34matrix.h"
#include "Tempest/cmath.h"

#include <storm.h>

#include <math.h>
#include <stdarg.h>
#include <stdio.h>

static void Console(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
}

static int  s_maxCategorySounds[SOUNDCATEGORIES_NUMCATEGORIES] = {0x7FFFFFFF, 1, 2};
static bool s_initialized;
static LISTDECLEX(Sound, link, s_soundListActive);
static LISTDECLEX(Sound, fadeLink, s_soundListFade);
static LISTDECLEX(Sound, updateLink, s_soundListUpdate);
static LISTDECLEX(Sound, panningLink, s_soundListPanning);
static LISTDECLEX(Sound, cutoffLink, s_soundListCutoff);
static float              s_musicVolume = 0.5f;
static float              s_soundVolume = 1.0f;
static float              s_masterVolume = 1.0f;
static float              s_soundMutingMultiplier = 1.0f;
static long               s_categoryCounts[SOUNDCATEGORIES_NUMCATEGORIES];
static MIDIPlayer         s_midiPlayer;
static NTempest::C3Vector s_listenerPosition;
static NTempest::C3Vector s_listenerFacing(1.0f, 0.0f, 0.0f);
static DWORD              s_lastHeartbeat;

bool (*Sound::m_positionUpdateCallback)(LONGLONG handle, NTempest::C3Vector &position);

static BOOL SoundIdle(LPCVOID, LPVOID) {
  DWORD now = OsGetAsyncTimeMsPrecise();
  if (now - s_lastHeartbeat > 50) {
    s_lastHeartbeat = now;
    Sound::Update();
  }
  return 1;
}

UINT SndGetCPUPerformance() {
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
      m_category(SOUNDCATEGORY_NONE),
      m_minDistance(1.0f),
      m_lastDistance(INFINITY),
      m_lastAzimuth(INFINITY),
      m_lastElevation(INFINITY) {
}

Sound::~Sound() {
  Stop();
  DecrementCategory(m_category);
}

int Sound::Initialize(bool (*GetParamInt)(LPCSTR, int &), bool (*GetParamFloat)(LPCSTR, float &), bool (*GetParamString)(LPCSTR, LPCSTR &)) {
  ASSERT(GetParamInt);
  ASSERT(GetParamFloat);
  ASSERT(GetParamString);

  if (s_initialized) {
    return 0;
  }

  g_streamingCache = new StreamingCache;
  GlobalAUGraph::Get().Initialize();
  LoaderThreadInitialize();
  EventRegister(EVENT_ID_IDLE, SoundIdle);

  memset(s_categoryCounts, 0, sizeof(s_categoryCounts));
  s_initialized = true;
  return 0;
}

void Sound::Shutdown() {
  while (Sound *sound = s_soundListActive.Head()) {
    delete sound;
  }

  if (s_initialized) {
    EventUnregister(EVENT_ID_IDLE, SoundIdle);
    s_midiPlayer.Reset();
    LoaderThreadCleanUp();
    GlobalAUGraph::Get().Destroy();
    delete g_streamingCache;
    g_streamingCache = 0;
    s_initialized = false;
  }
}

void Sound::Update() {
  ProcessStopList();
  ProcessFadeList();
  ProcessUpdateList();
  ProcessPanningList(s_listenerPosition);
  ProcessCutoffList(s_listenerPosition);
}

void Sound::ProcessStopList() {
  SAFEITERATELIST(Sound, s_soundListActive, sound) {
    if (!sound->m_stream) {
      if (!(sound->m_flags & SOUNDFLAG_KEEPALIVE)) {
        delete sound;
      }
    } else if (sound->m_channel != -1 && !sound->IsSuspended() && !((Player *)sound->m_stream)->IsPlaying()) {
      if (sound->m_flags & SOUNDFLAG_KEEPALIVE) {
        sound->Stop();
      } else {
        delete sound;
      }
    }
  }
}

void Sound::ProcessFadeList() {
  UINT timestamp = OsGetAsyncTimeMs();
  SAFEITERATELIST(Sound, s_soundListFade, sound) {
    if (sound->m_channel != -1) {
      ASSERT(sound->m_stream);
      UINT elapsed = timestamp - sound->m_fadeStartTime;
      int  volume = elapsed * sound->m_fadeRate;
      if (sound->m_fadeRate < 0.0f) {
        volume += sound->m_fadeVolume;
        if (volume <= 0) {
          sound->RemoveFromFadeList();
          if (sound->m_flags & ISOUNDFLAG_OUTOFRANGE) {
            sound->Suspend();
          } else if (sound->m_flags & SOUNDFLAG_KEEPALIVE) {
            sound->Stop();
          } else {
            delete sound;
          }
          continue;
        }
      }
      if (sound->m_fadeRate > 0.0f) {
        if (volume >= sound->m_fadeVolume) {
          volume = sound->m_fadeVolume;
          sound->RemoveFromFadeList();
        }
      }
      sound->SetVolume(volume);
    }
  }
}

void Sound::ProcessUpdateList() {
  if (!m_positionUpdateCallback) {
    return;
  }

  ITERATELIST(Sound, s_soundListUpdate, sound) {
    if (sound->m_channel != -1) {
      NTempest::C3Vector worldPosition;
      if (m_positionUpdateCallback(sound->m_updateHandle, worldPosition)) {
        sound->m_worldPosition = worldPosition;
      }
    }
  }
}

void Sound::ProcessPanningList(const NTempest::C3Vector &listenerPos) {
  ITERATELIST(Sound, s_soundListActive, sound) {
    sound->UpdatePosition();
  }
}

void Sound::ProcessCutoffList(const NTempest::C3Vector &listenerPos) {
  SAFEITERATELIST(Sound, s_soundListCutoff, sound) {
    if (!((sound->m_worldPosition - listenerPos).SquaredMag() > sound->m_cutoffDistanceSquared)) {
      if (sound->m_flags & ISOUNDFLAG_OUTOFRANGE) {
        sound->m_flags &= ~ISOUNDFLAG_OUTOFRANGE;
        sound->Resume();
        if (sound->m_flags & ISOUNDFLAG_ONFADELIST) {
          sound->SetFadeIn(2.0f, sound->m_fadeVolume * (1.0f / 255.0f));
        } else {
          sound->SetFadeIn(2.0f, sound->m_volume);
        }
      }
    } else {
      if (!(sound->m_flags & ISOUNDFLAG_OUTOFRANGE)) {
        if (sound->m_flags & ISOUNDFLAG_LOOPING) {
          sound->m_flags |= ISOUNDFLAG_OUTOFRANGE;
        }
        sound->Stop(2.0f);
      }
    }
  }
}

Sound *Sound::Alloc(LPCSTR name) {
  Sound *sound = new Sound;
  s_soundListActive.LinkNode(sound, LIST_TAIL, 0);
  if (name) {
    sound->m_fileNameHashed = SStrHash(name, 0, 0);
  }
  return sound;
}

Sound *Sound::Play(SOUNDCATEGORIES category, LPCSTR filename, UINT mode, bool startPaused, int flags) {
  return PlayLooped(category, filename, 0, mode, startPaused, flags);
}

Sound *Sound::PlayLooped(SOUNDCATEGORIES category, LPCSTR filename, int loopCount, UINT mode, bool startPaused, int flags) {
  ASSERT(loopCount >= -1);
  ASSERT(filename);

  if (!s_initialized) {
    return 0;
  }

  StreamPlayer *player;
  LPCSTR        extension = SStrChrR(filename, '.');
  if (extension && !SStrCmpI(extension, ".wav", 0x7FFFFFFF)) {
    player = Recycler::s_singleton.New<Player::e_wavPlayer>();
    player->SetLoopCount(loopCount + 1);
    ((WAVPlayer *)player)->SetWAVSource(filename);
  } else if (extension && !SStrCmpI(extension, ".mp3", 0x7FFFFFFF)) {
    player = Recycler::s_singleton.New<Player::e_mp3Player>();
    if (!((MP3Player *)player)->SetMP3Source(filename)) {
      Recycler::s_singleton.Delete(player);
      Console("[Sound::Play] The file \"%s\" could not be opened.", filename);
      return 0;
    }
    player->SetLoopCount(loopCount + 1);
  } else {
    Console("[Sound::Play] The file \"%s\" could not be opened.", filename);
    return 0;
  }

  Sound *sound = Alloc(filename);
  ASSERT(sound);
  sound->m_flags |= flags & 0xFF;
  sound->m_stream = player;
  sound->m_category = category;
  if (mode & 0x1000) {
    sound->m_flags |= ISOUNDFLAG_3D;
  }
  if (loopCount) {
    sound->m_flags |= ISOUNDFLAG_LOOPING;
  }

  if (!startPaused) {
    sound->m_channel = 0;
    sound->IncrementCategory(category);
    sound->UpdateVolume();
    sound->UpdatePosition();
    player->Start();
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
  return Play(category, filename, 0x1000, startPaused, flags);
}

Sound *Sound::Play2DLooped(SOUNDCATEGORIES category, LPCSTR filename, int flags, UINT loopCount, bool startPaused) {
  if (DupeCheckFailed(category, filename, flags)) {
    return 0;
  }
  if (loopCount == 1) {
    return Play2D(category, filename, flags, startPaused);
  }
  return PlayLooped(category, filename, loopCount - 1, 0x2002, startPaused, flags);
}

Sound *Sound::Play3DLooped(SOUNDCATEGORIES category, LPCSTR filename, int flags, UINT loopCount, bool startPaused) {
  if (DupeCheckFailed(category, filename, flags)) {
    return 0;
  }
  if (loopCount == 1) {
    return Play3D(category, filename, flags, startPaused);
  }
  return PlayLooped(category, filename, loopCount - 1, 0x1002, startPaused, flags);
}

void Sound::KillSound(Sound *&sound) {
  if (sound) {
    delete sound;
  }
  sound = 0;
}

void Sound::SetFadeIn(float fadeTime, float volume) {
  ASSERT(volume >= 0.0f && volume <= 1.0f);
  if (m_stream && !(fadeTime < 0.1f)) {
    m_fadeVolume = volume * 255.0f;
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
    m_fadeVolume = volume * 255.0f;
    m_fadeRate = m_fadeVolume / (float)fadeTime;
    if (!IsSuspended()) {
      m_fadeStartTime = OsGetAsyncTimeMs();
      AddToFadeList();
    }
  }
}

void Sound::Set3DUpdateHandle(LONGLONG handle) {
  if (!(m_flags & ISOUNDFLAG_3D)) {
    Console("[Sound::Set3DUpdateHandle] Tried to set update handle of \"%s\", which is not a 3D sound.", m_stream ? ((Player *)m_stream)->GetSource() : "");
    return;
  }
  if (m_channel == -1 || !m_stream) {
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
  return ((Player *)m_stream)->IsPlaying();
}

bool Sound::IsOutOfRange() {
  return (m_flags & ISOUNDFLAG_OUTOFRANGE) != 0;
}

void Sound::Suspend() {
  if (IsSuspended()) {
    return;
  }
  DecrementCategory(m_category);
  if (m_channel != -1) {
    ((Player *)m_stream)->Stop();
    m_channel = -1;
  }
  m_suspendedFlags = m_flags;
  m_flags |= ISOUNDFLAG_SUSPENDED;
  RemoveFromUpdateList();
  RemoveFromPanningList();
}

void Sound::Resume() {
  if (!IsSuspended()) {
    return;
  }
  m_channel = 0;
  IncrementCategory(m_category);
  if (m_suspendedFlags & ISOUNDFLAG_ONUPDATELIST) {
    AddToUpdateList();
  }
  if (m_suspendedFlags & ISOUNDFLAG_ONPANNINGLIST) {
    AddToPanningList();
  }
  m_flags &= ~ISOUNDFLAG_SUSPENDED;
  UpdatePosition();
  UpdateVolume();
  if (m_flags & ISOUNDFLAG_FREQSET) {
    SetFrequency(m_freq);
  }
  ((Player *)m_stream)->Start();
}

void Sound::Stop() {
  DecrementCategory(m_category);
  if (m_stream) {
    ((Player *)m_stream)->Stop();
    Recycler::s_singleton.Delete((StreamPlayer *)m_stream);
    m_stream = 0;
  }
  m_channel = -1;
  RemoveFromFadeList();
  RemoveFromUpdateList();
  RemoveFromPanningList();
  RemoveFromCutoffList();
}

void Sound::UpdateVolume() {
  if (m_channel != -1) {
    SetVolume(m_volume);
  }
}

void Sound::UpdatePosition() {
  if (!(m_flags & ISOUNDFLAG_3D) || m_channel == -1 || !m_stream) {
    return;
  }

  NTempest::C3Vector delta = m_worldPosition - s_listenerPosition;
  float              facing = atan2f(s_listenerFacing.y, s_listenerFacing.x);
  float              distance = sqrtf(delta.SquaredMag());
  if (distance < 0.01f) {
    distance = 0.01f;
  }
  float azimuth = atan2f(delta.y, delta.x);
  float cosine = delta.z / distance;
  if (cosine < -1.0f) {
    cosine = -1.0f;
  } else if (cosine > 1.0f) {
    cosine = 1.0f;
  }
  float elevation = acosf(cosine);
  if (distance < m_minDistance) {
    distance = m_minDistance;
  }
  distance /= m_minDistance;

  float relAzimuth = azimuth - facing;
  if (fabsf(distance - m_lastDistance) > 0.01f || fabsf(relAzimuth - m_lastAzimuth) > 0.01f || fabsf(elevation - m_lastElevation) > 0.01f) {
    ((Player *)m_stream)->Set3DPositionSpherical(distance, relAzimuth, elevation);
    m_lastDistance = distance;
    m_lastAzimuth = relAzimuth;
    m_lastElevation = elevation;
  }
}

void Sound::Stop(float fadeTime) {
  if (m_channel != -1 && m_stream && !(fadeTime < 0.1f)) {
    m_fadeVolume = GetVolume();
    m_fadeRate = m_fadeVolume / (fadeTime * -1000.0f);
    m_fadeStartTime = OsGetAsyncTimeMs();
    AddToFadeList();
  } else {
    Stop();
  }
}

void Sound::Stop(UINT fadeTime) {
  if (m_channel != -1 && m_stream && fadeTime) {
    m_fadeVolume = GetVolume();
    m_fadeRate = m_fadeVolume / -(float)fadeTime;
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
    UpdateVolume();
    UpdatePosition();
    if (state) {
      ((Player *)m_stream)->Stop();
    }
    return true;
  }
  if (state) {
    return true;
  }
  m_channel = 0;
  IncrementCategory(m_category);
  UpdateVolume();
  UpdatePosition();
  ((Player *)m_stream)->Start();
  return true;
}

void Sound::SetPosition(const NTempest::C3Vector &worldPosition, const NTempest::C3Vector *vel) {
  if (!(m_flags & ISOUNDFLAG_3D)) {
    Console("[Sound::SetPosition] Tried to set position of \"%s\", which is not a 3D sound.", m_stream ? ((Player *)m_stream)->GetSource() : "");
    return;
  }
  m_flags |= ISOUNDFLAG_POSITIONSET;
  m_worldPosition = worldPosition;
  if (vel) {
    m_flags |= ISOUNDFLAG_VELOCITYSET;
    m_velocity = *vel;
  }
  if (IsSuspended() || m_channel == -1) {
    return;
  }
  UpdatePosition();
}

void Sound::SetReverbProperties(const _FSOUND_REVERB_CHANNELPROPERTIES *reverb) {
}

void Sound::SetPanning(float pan) {
}

void Sound::SetCutoffDistanceSquared(float distanceSquared) {
  if (!(m_flags & ISOUNDFLAG_3D)) {
    return;
  }
  m_cutoffDistanceSquared = distanceSquared;
  if (distanceSquared != 0.0f) {
    AddToCutoffList();
  } else {
    RemoveFromCutoffList();
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
  m_flags |= ISOUNDFLAG_SETVOLUME;
  m_volume = volume;
  if (!IsSuspended() && m_channel != -1 && m_stream) {
    float categoryVolume = (m_flags & SOUNDFLAG_ISMUSIC) ? s_masterVolume * s_musicVolume : s_soundMutingMultiplier * s_soundVolume * s_masterVolume;
    ((Player *)m_stream)->SetVolume(volume * ((m_flags & ISOUNDFLAG_3D) ? 1.3333f : 1.0f) * categoryVolume);
  }
}

void Sound::SetFrequency(int freq) {
  m_freq = freq;
  m_flags |= ISOUNDFLAG_FREQSET;
  if (!m_stream) {
    return;
  }
  Player *player = (Player *)m_stream;
  if (player->Identify() != Player::e_wavPlayer) {
    Console("[Sound::SetFrequency] Don't know how to set frequency of \"%s\", which is player type %d", player->GetSource(), player->Identify());
    return;
  }
  ((WAVPlayer *)player)->SetFrequency(m_freq);
}

void Sound::SetDistances(float min, float max) {
  if (m_stream && (m_flags & ISOUNDFLAG_3D) && min > 0.0f) {
    m_minDistance = min;
    m_flags |= ISOUNDFLAG_MINMAXSET;
  }
}

void Sound::SetListenerAttributes(
    const NTempest::C3Vector &worldPosition,
    const NTempest::C3Vector *worldVelocity,
    const NTempest::C3Vector &worldForward,
    const NTempest::C3Vector &worldUp
) {
  s_listenerPosition = worldPosition;
  s_listenerFacing = worldForward;
}

void Sound::GetListenerPosition(NTempest::C3Vector &position) {
  position = s_listenerPosition;
}

void Sound::SetReverbProperties(const _FSOUND_REVERB_PROPERTIES *reverb) {
}

void Sound::AddToFadeList() {
  if (!(m_flags & ISOUNDFLAG_ONFADELIST)) {
    s_soundListFade.LinkNode(this, LIST_TAIL, 0);
    m_flags |= ISOUNDFLAG_ONFADELIST;
  }
}

void Sound::RemoveFromFadeList() {
  if (m_flags & ISOUNDFLAG_ONFADELIST) {
    SetVolume(m_fadeVolume);
    s_soundListFade.UnlinkNode(this);
    m_flags &= ~ISOUNDFLAG_ONFADELIST;
  }
}

void Sound::AddToUpdateList() {
  if (!(m_flags & ISOUNDFLAG_ONUPDATELIST)) {
    s_soundListUpdate.LinkNode(this, LIST_TAIL, 0);
    m_flags |= ISOUNDFLAG_ONUPDATELIST;
  }
}

void Sound::RemoveFromUpdateList() {
  if (m_flags & ISOUNDFLAG_ONUPDATELIST) {
    s_soundListUpdate.UnlinkNode(this);
    m_flags &= ~ISOUNDFLAG_ONUPDATELIST;
  }
}

void Sound::AddToPanningList() {
  if (!(m_flags & ISOUNDFLAG_ONPANNINGLIST)) {
    s_soundListPanning.LinkNode(this, LIST_TAIL, 0);
    m_flags |= ISOUNDFLAG_ONPANNINGLIST;
  }
}

void Sound::RemoveFromPanningList() {
  if (m_flags & ISOUNDFLAG_ONPANNINGLIST) {
    s_soundListPanning.UnlinkNode(this);
    m_flags &= ~ISOUNDFLAG_ONPANNINGLIST;
  }
}

void Sound::AddToCutoffList() {
  if (!(m_flags & ISOUNDFLAG_ONCUTOFFLIST)) {
    s_soundListCutoff.LinkNode(this, LIST_TAIL, 0);
    m_flags |= ISOUNDFLAG_ONCUTOFFLIST;
  }
}

void Sound::RemoveFromCutoffList() {
  if (m_flags & ISOUNDFLAG_ONCUTOFFLIST) {
    s_soundListCutoff.UnlinkNode(this);
    m_flags &= ~ISOUNDFLAG_ONCUTOFFLIST;
  }
}

void Sound::SetSoundVolume(float volume) {
  s_soundVolume = min(max(0.0f, volume), 1.0f);
  UpdateSoundVolumes(false);
}

void Sound::SetMusicVolume(float volume) {
  s_musicVolume = min(max(0.0f, volume), 1.0f);
  UpdateSoundVolumes(true);
}

void Sound::SetMasterVolume(float volume) {
  s_masterVolume = min(max(0.0f, volume), 1.0f);
  UpdateSoundVolumes(false);
  UpdateSoundVolumes(true);
}

void Sound::MuteSFX(bool m) {
  s_soundMutingMultiplier = m ? 0.0f : 1.0f;
  UpdateSoundVolumes(false);
}

void Sound::UpdateSoundVolumes(bool music) {
  SAFEITERATELIST(Sound, s_soundListActive, sound) {
    if (sound->m_channel != -1 && music == ((sound->m_flags & SOUNDFLAG_ISMUSIC) != 0)) {
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
  if (flags & SOUNDFLAG_NODUPLICATES) {
    UINT filenameHash = SStrHash(fileName, 0, 0);
    SAFEITERATELIST(Sound, s_soundListActive, sound) {
      if (filenameHash == sound->m_fileNameHashed && (!(sound->m_flags & ISOUNDFLAG_ONFADELIST) || (sound->m_flags & ISOUNDFLAG_OUTOFRANGE))) {
        return true;
      }
    }
  }
  return false;
}

bool Sound::IsSuspended() const {
  return (m_flags & ISOUNDFLAG_SUSPENDED) != 0;
}

void Sound::IncrementCategory(SOUNDCATEGORIES category) {
  if (!(m_flags & ISOUNDFLAG_CATEGORYCOUNTED)) {
    m_flags |= ISOUNDFLAG_CATEGORYCOUNTED;
    ++s_categoryCounts[category];
  }
}

void Sound::DecrementCategory(SOUNDCATEGORIES category) {
  if (m_flags & ISOUNDFLAG_CATEGORYCOUNTED) {
    m_flags &= ~ISOUNDFLAG_CATEGORYCOUNTED;
    if (s_categoryCounts[category]) {
      --s_categoryCounts[category];
    }
  }
}

int Sound::GetMixRate() {
  return 44100;
}

int Sound::MIDI_Initialize() {
  return 0;
}

void Sound::MIDI_Shutdown() {
  s_midiPlayer.Reset();
}

void Sound::MIDI_Play(LPCSTR midiFilename, LPCSTR dlsFilename) {
  if (!s_initialized) {
    return;
  }

  if (SStrCmp(midiFilename, s_midiPlayer.GetSource(), 0x7FFFFFFF) || SStrCmp(dlsFilename, s_midiPlayer.GetSourceDLS(), 0x7FFFFFFF)) {
    s_midiPlayer.Reset();
    s_midiPlayer.SetMIDISource(midiFilename);
    s_midiPlayer.SetDLSSource(dlsFilename);
    s_midiPlayer.Start();
  }
}

void Sound::MIDI_Stop() {
  s_midiPlayer.Reset();
}

void Sound::MIDI_SetVolume(float volume) {
  ASSERT(volume >= 0.0f && volume <= 1.0f);
  s_midiPlayer.SetVolume(volume);
}

bool Sound::MIDI_Playing() {
  return s_midiPlayer.IsPlaying();
}
