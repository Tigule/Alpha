#include "SoundSamplePreferencesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SoundSamplePreferencesRec::GetFilename() {
  return "DBFilesClient\\SoundSamplePreferences.dbc";
}

SoundSamplePreferencesRec::SoundSamplePreferencesRec() {
}

SoundSamplePreferencesRec::~SoundSamplePreferencesRec() {
}

bool SoundSamplePreferencesRec::Read(SFile *f, LPCSTR stringBuffer) {
  int error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_EAX1EffectLevel) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleDirect) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleDirectHF) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleRoom) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleRoomHF) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleObstruction) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleObstructionLFRatio) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleOcclusion) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleOcclusionLFRatio) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleOcclusionRoomRatio) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleRoomRolloff) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleAirAbsorption) == 0);
  error |= (SFileReadTyped(f, &m_EAX2SampleOutsideVolumeHF) == 0);
  error |= (SFileReadTyped(f, &m_EAX3SampleOcclusionDirectRatio) == 0);
  error |= (SFileReadTyped(f, &m_EAX3SampleExclusion) == 0);
  error |= (SFileReadTyped(f, &m_EAX3SampleExclusionLFRatio) == 0);
  error |= (SFileReadTyped(f, &m_EAX3SampleDopplerFactor) == 0);
  error |= (SFileReadTyped(f, &m_Fast2DPredelayTime) == 0);
  error |= (SFileReadTyped(f, &m_Fast2DDamping) == 0);
  error |= (SFileReadTyped(f, &m_Fast2DReverbTime) == 0);

  if (error) {
    ConsoleWrite("Error reading SoundSamplePreferencesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
