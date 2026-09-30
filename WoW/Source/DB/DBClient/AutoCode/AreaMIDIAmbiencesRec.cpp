#include "AreaMIDIAmbiencesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR AreaMIDIAmbiencesRec::GetFilename() {
  return "DBFilesClient\\AreaMIDIAmbiences.dbc";
}

AreaMIDIAmbiencesRec::AreaMIDIAmbiencesRec() {
}

AreaMIDIAmbiencesRec::~AreaMIDIAmbiencesRec() {
}

bool AreaMIDIAmbiencesRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempDaySequenceIndices[1];
  UINT tempNightSequenceIndices[1];
  UINT tempDLSFileIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &tempDaySequenceIndices[0]) ||
      !SFileReadTyped(f, &tempNightSequenceIndices[0]) ||
      !SFileReadTyped(f, &tempDLSFileIndices[0]) ||
      !SFileReadTyped(f, &m_volume)) {
    ConsoleWrite("Error reading AreaMIDIAmbiencesRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_DaySequence = &stringBuffer[tempDaySequenceIndices[0]];
    m_NightSequence = &stringBuffer[tempNightSequenceIndices[0]];
    m_DLSFile = &stringBuffer[tempDLSFileIndices[0]];
  } else {
    m_DaySequence = "";
    m_NightSequence = "";
    m_DLSFile = "";
  }

  return true;
}
