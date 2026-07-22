#pragma once

#include <DB/WowClientDB.h>

class SoundProviderPreferencesRec {
 public:
  SoundProviderPreferencesRec();
  ~SoundProviderPreferencesRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 26;
  }

  static unsigned int GetRowSize() {
    return 104;
  }

  int GetID() {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  const char *m_Description;
  int         m_Flags;
  int         m_EAXEnvironmentSelection;
  float       m_EAXEffectVolume;
  float       m_EAXDecayTime;
  float       m_EAXDamping;
  float       m_EAX2EnvironmentSize;
  float       m_EAX2EnvironmentDiffusion;
  int         m_EAX2Room;
  int         m_EAX2RoomHF;
  float       m_EAX2DecayHFRatio;
  int         m_EAX2Reflections;
  float       m_EAX2ReflectionsDelay;
  int         m_EAX2Reverb;
  float       m_EAX2ReverbDelay;
  float       m_EAX2RoomRolloff;
  float       m_EAX2AirAbsorption;
  int         m_EAX3RoomLF;
  float       m_EAX3DecayLFRatio;
  float       m_EAX3EchoTime;
  float       m_EAX3EchoDepth;
  float       m_EAX3ModulationTime;
  float       m_EAX3ModulationDepth;
  float       m_EAX3HFReference;
  float       m_EAX3LFReference;
};

extern WowClientDB<SoundProviderPreferencesRec> g_soundProviderPreferencesDB;
