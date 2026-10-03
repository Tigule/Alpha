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

  FILENAMEENTRY(const FILENAMEENTRY &rhs) {
    *this = rhs;
  }

  const FILENAMEENTRY &operator=(const FILENAMEENTRY &rhs) {
    if (this != &rhs) {
      UINT frequency = rhs.accumulatedFreq;
      SStrCopy(fileName, rhs.fileName, sizeof(fileName));
      accumulatedFreq = frequency;
    }
    return *this;
  }

  void SetName(LPCSTR name, UINT frequency) {
    SStrCopy(fileName, name, sizeof(fileName));
    accumulatedFreq = frequency;
  }

  char fileName[MAX_PATH];
  UINT accumulatedFreq;
};

struct WEAPONSOUNDS {
  WEAPONSOUNDS();
  WEAPONSOUNDS(const WEAPONSOUNDS &rhs);

  const WEAPONSOUNDS &operator=(const WEAPONSOUNDS &rhs);
  ~WEAPONSOUNDS();
  void                Clear();

  UINT soundList[2];
};

struct IMPACTSOUNDDESC {
  IMPACTSOUNDDESC() {
  }

  ~IMPACTSOUNDDESC();

  WEAPONSOUNDS materialSounds[2];
};

struct IMPACTSOUNDARRAY {
  IMPACTSOUNDDESC desc[10];
};

struct Sound;

struct SOUNDDEFINITION : public TSHashObject<SOUNDDEFINITION, HASHKEY_NONE> {
 public:
  TSCArray<FILENAMEENTRY, 10> m_fileNames;
  float                       m_volume;
  float                       m_pitch;
  float                       m_pitchVariation;
  UINT                        m_priority;
  UINT                        m_channel;
  UINT                        m_flags;
  float                       m_minDistance;
  float                       m_maxDistance;
  float                       m_distanceCutoffSquared;
  UINT                        m_totalFrequency;
  UINT                        m_lastPlayed;
  UINT                        m_loopCounter;
  UINT                        m_primeStepIndex;
  int                         m_equalFreqs;
  int                         m_reverbPrefIndex;

  SOUNDDEFINITION();
  SOUNDDEFINITION(const SOUNDDEFINITION &rhs);
  ~SOUNDDEFINITION();

  const SOUNDDEFINITION &operator=(const SOUNDDEFINITION &rhs);

  LPCSTR GetRandomFileName(int index);
  void   SetFrequencyAndVolume(Sound *sound, float volumeScaler, bool neverVaryVolume) const;
  void   Set3DParams(Sound *sound, const NTempest::C3Vector *pos);
  int    GetOsFlags() const;

 private:
  float GetVolume(float volumeScale, bool neverVary) const;
  void  Clear();

};

struct SHEATHSOUNDHASH : public TSHashObject<SHEATHSOUNDHASH, HASHKEY_NONE> {
  TSFixedArray<UINT> materialSheathSound;
  TSFixedArray<UINT> materialUnsheathSound;
};

struct UISOUNDLOOKUP : public TSHashObject<UISOUNDLOOKUP, HASHKEY_STRI> {
  UINT soundID;
};

struct REVERBINFO {
  BYTE                             inUse;
  _FSOUND_REVERB_CHANNELPROPERTIES prefs;

  REVERBINFO() : inUse(0) {
  }
};

extern TSHashTable<SHEATHSOUNDHASH, HASHKEY_NONE> g_sheathSoundList;
extern TSHashTable<UISOUNDLOOKUP, HASHKEY_STRI>   g_uiSoundLookups;
extern TSFixedArray<IMPACTSOUNDARRAY>             g_impactSounds;
extern WEAPONSOUNDS                               g_weaponSwingSounds[3];

SOUNDDEFINITION                  *ISndInterfaceGetSndEntry(UINT soundID);
_FSOUND_REVERB_CHANNELPROPERTIES *GetReverbType(int index);

#endif
