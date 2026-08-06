#pragma once

#include <DB/WowClientDB.h>

class SoundSamplePreferencesRec {
 public:
  SoundSamplePreferencesRec();
  ~SoundSamplePreferencesRec();

  static LPCSTR GetFilename();

  static UINT GetNumColumns() {
    return 21;
  }

  static UINT GetRowSize() {
    return 84;
  }

  int GetID() const {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, LPCSTR stringBuffer);

  int   m_ID;
  float m_EAX1EffectLevel;
  int   m_EAX2SampleDirect;
  int   m_EAX2SampleDirectHF;
  int   m_EAX2SampleRoom;
  int   m_EAX2SampleRoomHF;
  float m_EAX2SampleObstruction;
  float m_EAX2SampleObstructionLFRatio;
  float m_EAX2SampleOcclusion;
  float m_EAX2SampleOcclusionLFRatio;
  float m_EAX2SampleOcclusionRoomRatio;
  float m_EAX2SampleRoomRolloff;
  float m_EAX2SampleAirAbsorption;
  int   m_EAX2SampleOutsideVolumeHF;
  float m_EAX3SampleOcclusionDirectRatio;
  float m_EAX3SampleExclusion;
  float m_EAX3SampleExclusionLFRatio;
  float m_EAX3SampleDopplerFactor;
  float m_Fast2DPredelayTime;
  float m_Fast2DDamping;
  float m_Fast2DReverbTime;
};

extern WowClientDB<SoundSamplePreferencesRec> g_soundSamplePreferencesDB;
