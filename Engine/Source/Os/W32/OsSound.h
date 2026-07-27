#pragma once

#include "Tempest/c3vector.h"

#include <stpl.h>

enum SOUNDCATEGORIES {
  SOUNDCATEGORY_NONE = 0,
  SOUNDCATEGORY_VOCALUI = 1,
  SOUNDCATEGORY_SPLASHES = 2,
  SOUNDCATEGORIES_NUMCATEGORIES = 3
};

enum SNDROOMTYPE {
  SNDROOMTYPE_GENERIC = 0,
  SNDROOMTYPE_PADDEDCELL = 1,
  SNDROOMTYPE_ROOM = 2,
  SNDROOMTYPE_BATHROOM = 3,
  SNDROOMTYPE_LIVINGROOM = 4,
  SNDROOMTYPE_STONEROOM = 5,
  SNDROOMTYPE_AUDITORIUM = 6,
  SNDROOMTYPE_CONCERTHALL = 7,
  SNDROOMTYPE_CAVE = 8,
  SNDROOMTYPE_ARENA = 9,
  SNDROOMTYPE_HANGAR = 10,
  SNDROOMTYPE_CARPETEDHALLWAY = 11,
  SNDROOMTYPE_HALLWAY = 12,
  SNDROOMTYPE_STONECORRIDOR = 13,
  SNDROOMTYPE_ALLEY = 14,
  SNDROOMTYPE_FOREST = 15,
  SNDROOMTYPE_CITY = 16,
  SNDROOMTYPE_MOUNTAINS = 17,
  SNDROOMTYPE_QUARRY = 18,
  SNDROOMTYPE_PLAIN = 19,
  SNDROOMTYPE_PARKINGLOT = 20,
  SNDROOMTYPE_SEWERPIPE = 21,
  SNDROOMTYPE_UNDERWATER = 22,
  SNDROOMTYPE_DRUGGED = 23,
  SNDROOMTYPE_DIZZY = 24,
  SNDROOMTYPE_PSYCHOTIC = 25,
  NUM_SNDROOMTYPES = 26
};

struct _FSOUND_REVERB_PROPERTIES {
  unsigned int Environment;
  float        EnvSize;
  float        EnvDiffusion;
  int          Room;
  int          RoomHF;
  int          RoomLF;
  float        DecayTime;
  float        DecayHFRatio;
  float        DecayLFRatio;
  int          Reflections;
  float        ReflectionsDelay;
  float        ReflectionsPan[3];
  int          Reverb;
  float        ReverbDelay;
  float        ReverbPan[3];
  float        EchoTime;
  float        EchoDepth;
  float        ModulationTime;
  float        ModulationDepth;
  float        AirAbsorptionHF;
  float        HFReference;
  float        LFReference;
  float        RoomRolloffFactor;
  float        Diffusion;
  float        Density;
  unsigned int Flags;
};

struct FSOUND_STREAM;
struct FSOUND_SAMPLE;
struct _FSOUND_REVERB_CHANNELPROPERTIES;

NODEDECL(Sound) {
 public:
  Sound();
  ~Sound();

  static int Initialize(
      bool(*GetParamInt)(const char *, int &),
      bool(*GetParamFloat)(const char *, float &),
      bool(*GetParamString)(const char *, const char *&)
  );
  static void Shutdown();
  static void SetSoundVolume(float volume);
  static void SetMusicVolume(float volume);
  static void SetMasterVolume(float volume);
  static void MuteSFX(bool m);
  static int MIDI_Initialize();
  static void MIDI_Shutdown();
  static void MIDI_Play(const char *midiFilename, const char *dlsFilename);
  static void MIDI_Stop();
  static void MIDI_SetVolume(float volume);
  static bool MIDI_Playing();
  static void Update();
  static void GetListenerPosition(NTempest::C3Vector &position);
  static void SetListenerAttributes(
      const NTempest::C3Vector &worldPosition,
      const NTempest::C3Vector *worldVelocity,
      const NTempest::C3Vector &worldForward,
      const NTempest::C3Vector &worldUp
  );
  static Sound *Play2D(SOUNDCATEGORIES category, const char *filename, int flags, bool startPaused);
  static Sound *Play3D(SOUNDCATEGORIES category, const char *filename, int flags, bool startPaused);
  static Sound *Play2DLooped(SOUNDCATEGORIES category, const char *filename, int flags, unsigned int loopCount, bool startPaused);
  static Sound *Play3DLooped(SOUNDCATEGORIES category, const char *filename, int flags, unsigned int loopCount, bool startPaused);
  static void KillSound(Sound *&sound);
  static void SetReverbProperties(const _FSOUND_REVERB_PROPERTIES *reverb);

  void Stop(float fadeTime);
  void Stop(unsigned int fadeTime);
  void SetFadeIn(float fadeTime, float volume);
  void SetFadeIn(unsigned int fadeTime, float volume);
  bool IsPlaying();
  bool IsStopping();
  bool IsOutOfRange();
  void Set3DUpdateHandle(__int64 handle);
  bool SetPaused(bool state);
  int  GetLengthMs();
  int  SetPositionMs(int milliseconds);
  void SetPosition(const NTempest::C3Vector &worldPosition, const NTempest::C3Vector *vel);
  void SetReverbProperties(const _FSOUND_REVERB_CHANNELPROPERTIES *reverb);
  void SetPanning(float pan);
  void SetCutoffDistanceSquared(float distanceSquared);
  void SetFrequency(int freq);
  void SetDistances(float min, float max);
  void SetVolume(float volume);

  static int GetNumOutputSystems();
  static const char *GetOutputSystemName(int index);
  static int GetNumDrivers();
  static const char *GetDriverName(int index);
  static int GetNumMixers();
  static const char *GetMixerName(int index);
  static int GetMixRate();
  static void SetPositionUpdateCallback(
      unsigned char(*callback)(__int64 handle, NTempest::C3Vector &position)
  ) {
    m_positionUpdateCallback = callback;
  }

  TSLink<Sound> link;
  TSLink<Sound> fadeLink;
  TSLink<Sound> updateLink;
  TSLink<Sound> panningLink;
  TSLink<Sound> cutoffLink;
  TSLink<Sound> stopLink;

 private:
  static Sound *Alloc(const char *name);
  static Sound *Play(SOUNDCATEGORIES category, const char *filename, unsigned int mode, bool startPaused, int flags);
  static Sound *PlayLooped(SOUNDCATEGORIES category, const char *filename, int loopCount, unsigned int mode, bool startPaused, int flags);
  static bool DupeCheckFailed(SOUNDCATEGORIES category, const char *fileName, int flags);
  static void ProcessStopList();
  static void ProcessFadeList();
  static void ProcessUpdateList();
  static void ProcessPanningList(const NTempest::C3Vector &listenerPos);
  static void ProcessCutoffList(const NTempest::C3Vector &listenerPos);
  void                     Stop();
  int                      GetVolume();
  void                     SetVolume(int volume);
  static void UpdateSoundVolumes(bool music);
  void                     UpdateVolume();
  void                     UpdatePosition();
  void                     AddToFadeList();
  void                     AddToUpdateList();
  void                     AddToPanningList();
  void                     AddToCutoffList();
  void                     RemoveFromFadeList();
  void                     RemoveFromUpdateList();
  void                     RemoveFromPanningList();
  void                     RemoveFromCutoffList();
  void                     IncrementCategory(SOUNDCATEGORIES category);
  void                     DecrementCategory(SOUNDCATEGORIES category);
  bool                     IsSuspended() const;
  void                     Suspend();
  void                     Resume();

 public:
  static unsigned char(*m_positionUpdateCallback)(__int64 handle, NTempest::C3Vector &position);

 private:
  int                m_channel;
  FSOUND_STREAM     *m_stream;
  unsigned int       m_flags;
  unsigned int       m_suspendedFlags;
  unsigned int       m_fadeStartTime;
  int                m_fadeVolume;
  float              m_fadeRate;
  float              m_panning;
  NTempest::C3Vector m_worldPosition;
  NTempest::C3Vector m_velocity;
  __int64            m_updateHandle;
  float              m_cutoffDistanceSquared;
  float              m_volume;
  unsigned long      m_freq;
  int                m_fileNameHashed;
  SOUNDCATEGORIES    m_category;
};

void SndSetObstructionCallback(float(*callback)(const NTempest::C3Vector &, const NTempest::C3Vector &));
