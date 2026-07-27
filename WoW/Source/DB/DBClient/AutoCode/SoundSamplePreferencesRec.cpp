#include "SoundSamplePreferencesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *SoundSamplePreferencesRec::GetFilename() {
  return "DBFilesClient\\SoundSamplePreferences.dbc";
}

SoundSamplePreferencesRec::SoundSamplePreferencesRec() {
}

SoundSamplePreferencesRec::~SoundSamplePreferencesRec() {
}

bool SoundSamplePreferencesRec::Read(SFile *f, const char *stringBuffer) {
  bool result = true;

  result = SFileReadTyped(f, &m_ID) && result;
  result = SFileReadTyped(f, &m_EAX1EffectLevel) && result;
  result = SFileReadTyped(f, &m_EAX2SampleDirect) && result;
  result = SFileReadTyped(f, &m_EAX2SampleDirectHF) && result;
  result = SFileReadTyped(f, &m_EAX2SampleRoom) && result;
  result = SFileReadTyped(f, &m_EAX2SampleRoomHF) && result;
  result = SFileReadTyped(f, &m_EAX2SampleObstruction) && result;
  result = SFileReadTyped(f, &m_EAX2SampleObstructionLFRatio) && result;
  result = SFileReadTyped(f, &m_EAX2SampleOcclusion) && result;
  result = SFileReadTyped(f, &m_EAX2SampleOcclusionLFRatio) && result;
  result = SFileReadTyped(f, &m_EAX2SampleOcclusionRoomRatio) && result;
  result = SFileReadTyped(f, &m_EAX2SampleRoomRolloff) && result;
  result = SFileReadTyped(f, &m_EAX2SampleAirAbsorption) && result;
  result = SFileReadTyped(f, &m_EAX2SampleOutsideVolumeHF) && result;
  result = SFileReadTyped(f, &m_EAX3SampleOcclusionDirectRatio) && result;
  result = SFileReadTyped(f, &m_EAX3SampleExclusion) && result;
  result = SFileReadTyped(f, &m_EAX3SampleExclusionLFRatio) && result;
  result = SFileReadTyped(f, &m_EAX3SampleDopplerFactor) && result;
  result = SFileReadTyped(f, &m_Fast2DPredelayTime) && result;
  result = SFileReadTyped(f, &m_Fast2DDamping) && result;
  result = SFileReadTyped(f, &m_Fast2DReverbTime) && result;

  if (!result) {
    ConsoleWrite("Error reading SoundSamplePreferencesRec", DEFAULT_COLOR);
  }

  return result;
}
