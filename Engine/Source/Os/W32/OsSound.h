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

struct Sound : public TSLinkedNode<Sound> {
 public:
  Sound();
  ~Sound();

  static int __fastcall Initialize(
      bool(__fastcall *GetParamInt)(const char *, int &),
      bool(__fastcall *GetParamFloat)(const char *, float &),
      bool(__fastcall *GetParamString)(const char *, const char *&)
  );
  static void __fastcall Shutdown();
  static void __fastcall SetSoundVolume(float volume);
  static void __fastcall SetMusicVolume(float volume);
  static void __fastcall SetMasterVolume(float volume);
  static void __fastcall MuteSFX(bool m);
  static int __fastcall MIDI_Initialize();
  static void __fastcall MIDI_Shutdown();
  static void __fastcall MIDI_Play(const char *midiFilename, const char *dlsFilename);
  static void __fastcall MIDI_Stop();
  static void __fastcall MIDI_SetVolume(float volume);
  static void __fastcall Update();
  static void __fastcall GetListenerPosition(NTempest::C3Vector &position);
  static void __fastcall SetListenerAttributes(
      const NTempest::C3Vector &worldPosition,
      const NTempest::C3Vector *worldVelocity,
      const NTempest::C3Vector &worldForward,
      const NTempest::C3Vector &worldUp
  );
  static Sound *__fastcall Play2D(SOUNDCATEGORIES category, const char *filename, int flags, bool startPaused);
  static Sound *__fastcall Play3D(SOUNDCATEGORIES category, const char *filename, int flags, bool startPaused);
  static Sound *__fastcall Play2DLooped(SOUNDCATEGORIES category, const char *filename, int flags, unsigned int loopCount, bool startPaused);
  static Sound *__fastcall Play3DLooped(SOUNDCATEGORIES category, const char *filename, int flags, unsigned int loopCount, bool startPaused);
  static void __fastcall   KillSound(Sound *&sound);
  static void __fastcall   SetReverbProperties(const _FSOUND_REVERB_PROPERTIES *reverb);

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

  static int __fastcall         GetNumOutputSystems();
  static const char *__fastcall GetOutputSystemName(int index);
  static int __fastcall         GetNumDrivers();
  static const char *__fastcall GetDriverName(int index);
  static int __fastcall         GetNumMixers();
  static const char *__fastcall GetMixerName(int index);
  static int __fastcall         GetMixRate();

  TSLink<Sound> link;
  TSLink<Sound> fadeLink;
  TSLink<Sound> updateLink;
  TSLink<Sound> panningLink;
  TSLink<Sound> cutoffLink;
  TSLink<Sound> stopLink;

 private:
  static Sound *__fastcall Alloc(const char *name);
  static Sound *__fastcall Play(SOUNDCATEGORIES category, const char *filename, unsigned int mode, bool startPaused, int flags);
  static Sound *__fastcall PlayLooped(SOUNDCATEGORIES category, const char *filename, int loopCount, unsigned int mode, bool startPaused, int flags);
  static bool __fastcall   DupeCheckFailed(SOUNDCATEGORIES category, const char *fileName, int flags);
  static void __fastcall   ProcessStopList();
  static void __fastcall   ProcessFadeList();
  static void __fastcall   ProcessUpdateList();
  static void __fastcall   ProcessPanningList(const NTempest::C3Vector &listenerPos);
  static void __fastcall   ProcessCutoffList(const NTempest::C3Vector &listenerPos);
  void                     Stop();
  int                      GetVolume();
  void                     SetVolume(int volume);
  static void __fastcall   UpdateSoundVolumes(bool music);
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
  static unsigned char(__fastcall *m_positionUpdateCallback)(__int64 handle, NTempest::C3Vector &position);

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

void __fastcall SndSetObstructionCallback(float(__fastcall *callback)(const NTempest::C3Vector &, const NTempest::C3Vector &));
