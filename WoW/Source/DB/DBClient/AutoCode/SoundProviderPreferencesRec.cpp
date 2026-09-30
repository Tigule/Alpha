#include "SoundProviderPreferencesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SoundProviderPreferencesRec::GetFilename() {
  return "DBFilesClient\\SoundProviderPreferences.dbc";
}

SoundProviderPreferencesRec::SoundProviderPreferencesRec() {
}

SoundProviderPreferencesRec::~SoundProviderPreferencesRec() {
}

bool SoundProviderPreferencesRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempDescriptionIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempDescriptionIndices[0]) ||
      !SFileReadTyped(f, &m_Flags) ||
      !SFileReadTyped(f, &m_EAXEnvironmentSelection) ||
      !SFileReadTyped(f, &m_EAXEffectVolume) ||
      !SFileReadTyped(f, &m_EAXDecayTime) ||
      !SFileReadTyped(f, &m_EAXDamping) ||
      !SFileReadTyped(f, &m_EAX2EnvironmentSize) ||
      !SFileReadTyped(f, &m_EAX2EnvironmentDiffusion) ||
      !SFileReadTyped(f, &m_EAX2Room) ||
      !SFileReadTyped(f, &m_EAX2RoomHF) ||
      !SFileReadTyped(f, &m_EAX2DecayHFRatio) ||
      !SFileReadTyped(f, &m_EAX2Reflections) ||
      !SFileReadTyped(f, &m_EAX2ReflectionsDelay) ||
      !SFileReadTyped(f, &m_EAX2Reverb) ||
      !SFileReadTyped(f, &m_EAX2ReverbDelay) ||
      !SFileReadTyped(f, &m_EAX2RoomRolloff) ||
      !SFileReadTyped(f, &m_EAX2AirAbsorption) ||
      !SFileReadTyped(f, &m_EAX3RoomLF) ||
      !SFileReadTyped(f, &m_EAX3DecayLFRatio) ||
      !SFileReadTyped(f, &m_EAX3EchoTime) ||
      !SFileReadTyped(f, &m_EAX3EchoDepth) ||
      !SFileReadTyped(f, &m_EAX3ModulationTime) ||
      !SFileReadTyped(f, &m_EAX3ModulationDepth) ||
      !SFileReadTyped(f, &m_EAX3HFReference) ||
      !SFileReadTyped(f, &m_EAX3LFReference)) {
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
