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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &tempDescriptionIndices[0]) == 0);
  error |= (SFileReadTyped(f, &m_Flags) == 0);
  error |= (SFileReadTyped(f, &m_EAXEnvironmentSelection) == 0);
  error |= (SFileReadTyped(f, &m_EAXEffectVolume) == 0);
  error |= (SFileReadTyped(f, &m_EAXDecayTime) == 0);
  error |= (SFileReadTyped(f, &m_EAXDamping) == 0);
  error |= (SFileReadTyped(f, &m_EAX2EnvironmentSize) == 0);
  error |= (SFileReadTyped(f, &m_EAX2EnvironmentDiffusion) == 0);
  error |= (SFileReadTyped(f, &m_EAX2Room) == 0);
  error |= (SFileReadTyped(f, &m_EAX2RoomHF) == 0);
  error |= (SFileReadTyped(f, &m_EAX2DecayHFRatio) == 0);
  error |= (SFileReadTyped(f, &m_EAX2Reflections) == 0);
  error |= (SFileReadTyped(f, &m_EAX2ReflectionsDelay) == 0);
  error |= (SFileReadTyped(f, &m_EAX2Reverb) == 0);
  error |= (SFileReadTyped(f, &m_EAX2ReverbDelay) == 0);
  error |= (SFileReadTyped(f, &m_EAX2RoomRolloff) == 0);
  error |= (SFileReadTyped(f, &m_EAX2AirAbsorption) == 0);
  error |= (SFileReadTyped(f, &m_EAX3RoomLF) == 0);
  error |= (SFileReadTyped(f, &m_EAX3DecayLFRatio) == 0);
  error |= (SFileReadTyped(f, &m_EAX3EchoTime) == 0);
  error |= (SFileReadTyped(f, &m_EAX3EchoDepth) == 0);
  error |= (SFileReadTyped(f, &m_EAX3ModulationTime) == 0);
  error |= (SFileReadTyped(f, &m_EAX3ModulationDepth) == 0);
  error |= (SFileReadTyped(f, &m_EAX3HFReference) == 0);
  error |= (SFileReadTyped(f, &m_EAX3LFReference) == 0);

  if (error) {
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
