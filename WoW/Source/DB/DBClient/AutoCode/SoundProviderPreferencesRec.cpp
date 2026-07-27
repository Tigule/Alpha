#include "SoundProviderPreferencesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *SoundProviderPreferencesRec::GetFilename() {
  return "DBFilesClient\\SoundProviderPreferences.dbc";
}

SoundProviderPreferencesRec::SoundProviderPreferencesRec() {
}

SoundProviderPreferencesRec::~SoundProviderPreferencesRec() {
}

bool SoundProviderPreferencesRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempDescriptionIndices[1];

  result = SFileReadTyped(f, &m_ID) && result;
  result = SFileReadTyped(f, &tempDescriptionIndices[0]) && result;
  result = SFileReadTyped(f, &m_Flags) && result;
  result = SFileReadTyped(f, &m_EAXEnvironmentSelection) && result;
  result = SFileReadTyped(f, &m_EAXEffectVolume) && result;
  result = SFileReadTyped(f, &m_EAXDecayTime) && result;
  result = SFileReadTyped(f, &m_EAXDamping) && result;
  result = SFileReadTyped(f, &m_EAX2EnvironmentSize) && result;
  result = SFileReadTyped(f, &m_EAX2EnvironmentDiffusion) && result;
  result = SFileReadTyped(f, &m_EAX2Room) && result;
  result = SFileReadTyped(f, &m_EAX2RoomHF) && result;
  result = SFileReadTyped(f, &m_EAX2DecayHFRatio) && result;
  result = SFileReadTyped(f, &m_EAX2Reflections) && result;
  result = SFileReadTyped(f, &m_EAX2ReflectionsDelay) && result;
  result = SFileReadTyped(f, &m_EAX2Reverb) && result;
  result = SFileReadTyped(f, &m_EAX2ReverbDelay) && result;
  result = SFileReadTyped(f, &m_EAX2RoomRolloff) && result;
  result = SFileReadTyped(f, &m_EAX2AirAbsorption) && result;
  result = SFileReadTyped(f, &m_EAX3RoomLF) && result;
  result = SFileReadTyped(f, &m_EAX3DecayLFRatio) && result;
  result = SFileReadTyped(f, &m_EAX3EchoTime) && result;
  result = SFileReadTyped(f, &m_EAX3EchoDepth) && result;
  result = SFileReadTyped(f, &m_EAX3ModulationTime) && result;
  result = SFileReadTyped(f, &m_EAX3ModulationDepth) && result;
  result = SFileReadTyped(f, &m_EAX3HFReference) && result;
  result = SFileReadTyped(f, &m_EAX3LFReference) && result;

  if (!result) {
    ConsoleWrite("Error reading SoundProviderPreferencesRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_Description = stringBuffer + tempDescriptionIndices[0];
  } else {
    m_Description = "";
  }

  return true;
}
