#ifndef WOW_SOURCE_SOUNDINTERFACE_ISOUNDINTERFACE_H
#define WOW_SOURCE_SOUNDINTERFACE_ISOUNDINTERFACE_H

#include <stpl.h>

namespace NTempest {
  class C3Vector;
}

struct _FSOUND_REVERB_CHANNELPROPERTIES {
  int   Direct;
  int   DirectHF;
  int   Room;
  int   RoomHF;
  int   Obstruction;
  float ObstructionLFRatio;
  int   Occlusion;
  float OcclusionLFRatio;
  float OcclusionRoomRatio;
  float OcclusionDirectRatio;
  int   Exclusion;
  float ExclusionLFRatio;
  int   OutsideVolumeHF;
  float DopplerFactor;
  float RolloffFactor;
  float RoomRolloffFactor;
  float AirAbsorptionFactor;
  int   Flags;
};

struct FILENAMEENTRY {
  FILENAMEENTRY() {
    fileName[0] = 0;
  }

  void SetName(const char *name, unsigned int frequency) {
    SStrCopy(fileName, name, sizeof(fileName));
    accumulatedFreq = frequency;
  }

  char         fileName[260];
  unsigned int accumulatedFreq;
};

struct WEAPONSOUNDS {
  WEAPONSOUNDS();
  WEAPONSOUNDS(const WEAPONSOUNDS &rhs);
  ~WEAPONSOUNDS();

  const WEAPONSOUNDS &operator=(const WEAPONSOUNDS &rhs);
  void                Clear();

  unsigned int soundList[2];
};

struct IMPACTSOUNDDESC {
  ~IMPACTSOUNDDESC();

  WEAPONSOUNDS materialSounds[2];
};

struct IMPACTSOUNDARRAY {
  IMPACTSOUNDDESC desc[10];
};

struct Sound;

struct SOUNDDEFINITION : public TSHashObject<SOUNDDEFINITION, HASHKEY_NONE> {
 public:
  SOUNDDEFINITION();
  SOUNDDEFINITION(const SOUNDDEFINITION &rhs);
  ~SOUNDDEFINITION();

  const SOUNDDEFINITION &operator=(const SOUNDDEFINITION &rhs);

  const char *GetRandomFileName(int index);
  int         GetOsFlags() const;
  void        Set3DParams(Sound *sound, const NTempest::C3Vector *pos);
  void        SetFrequencyAndVolume(Sound *sound, float volumeScaler, bool neverVaryVolume) const;

 private:
  float GetVolume(float volumeScale, bool neverVary) const;
  void  Clear();

 public:
  TSCArray<FILENAMEENTRY, 10> m_fileNames;
  float                       m_volume;
  float                       m_pitch;
  float                       m_pitchVariation;
  unsigned int                m_priority;
  unsigned int                m_channel;
  unsigned int                m_flags;
  float                       m_minDistance;
  float                       m_maxDistance;
  float                       m_distanceCutoffSquared;
  unsigned int                m_totalFrequency;
  unsigned int                m_lastPlayed;
  unsigned int                m_loopCounter;
  unsigned int                m_primeStepIndex;
  int                         m_equalFreqs;
  int                         m_reverbPrefIndex;
};

struct SHEATHSOUNDHASH : public TSHashObject<SHEATHSOUNDHASH, HASHKEY_NONE> {
  TSFixedArray<unsigned int> materialSheathSound;
  TSFixedArray<unsigned int> materialUnsheathSound;
};

struct UISOUNDLOOKUP : public TSHashObject<UISOUNDLOOKUP, HASHKEY_STRI> {
  unsigned int soundID;
};

struct REVERBINFO {
  REVERBINFO() : inUse(0) {
  }

  unsigned char                    inUse;
  _FSOUND_REVERB_CHANNELPROPERTIES prefs;
};

extern TSHashTable<SHEATHSOUNDHASH, HASHKEY_NONE> g_sheathSoundList;
extern TSHashTable<UISOUNDLOOKUP, HASHKEY_STRI>   g_uiSoundLookups;
extern TSFixedArray<IMPACTSOUNDARRAY>             g_impactSounds;
extern WEAPONSOUNDS                               g_weaponSwingSounds[3];

SOUNDDEFINITION *ISndInterfaceGetSndEntry(unsigned int soundID);
_FSOUND_REVERB_CHANNELPROPERTIES *GetReverbType(int index);

#endif
