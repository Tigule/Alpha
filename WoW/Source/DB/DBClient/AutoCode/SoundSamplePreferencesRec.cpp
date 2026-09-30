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

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_EAX1EffectLevel) ||
      !SFileReadTyped(f, &m_EAX2SampleDirect) ||
      !SFileReadTyped(f, &m_EAX2SampleDirectHF) ||
      !SFileReadTyped(f, &m_EAX2SampleRoom) ||
      !SFileReadTyped(f, &m_EAX2SampleRoomHF) ||
      !SFileReadTyped(f, &m_EAX2SampleObstruction) ||
      !SFileReadTyped(f, &m_EAX2SampleObstructionLFRatio) ||
      !SFileReadTyped(f, &m_EAX2SampleOcclusion) ||
      !SFileReadTyped(f, &m_EAX2SampleOcclusionLFRatio) ||
      !SFileReadTyped(f, &m_EAX2SampleOcclusionRoomRatio) ||
      !SFileReadTyped(f, &m_EAX2SampleRoomRolloff) ||
      !SFileReadTyped(f, &m_EAX2SampleAirAbsorption) ||
      !SFileReadTyped(f, &m_EAX2SampleOutsideVolumeHF) ||
      !SFileReadTyped(f, &m_EAX3SampleOcclusionDirectRatio) ||
      !SFileReadTyped(f, &m_EAX3SampleExclusion) ||
      !SFileReadTyped(f, &m_EAX3SampleExclusionLFRatio) ||
      !SFileReadTyped(f, &m_EAX3SampleDopplerFactor) ||
      !SFileReadTyped(f, &m_Fast2DPredelayTime) ||
      !SFileReadTyped(f, &m_Fast2DDamping) ||
      !SFileReadTyped(f, &m_Fast2DReverbTime)) {
    ConsoleWrite("Error reading SoundSamplePreferencesRec", DEFAULT_COLOR);
    return false;
  }

  return true;
}
