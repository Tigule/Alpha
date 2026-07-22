#include "AreaMIDIAmbiencesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *__fastcall AreaMIDIAmbiencesRec::GetFilename() {
  return "DBFilesClient\\AreaMIDIAmbiences.dbc";
}

AreaMIDIAmbiencesRec::AreaMIDIAmbiencesRec() {
}

AreaMIDIAmbiencesRec::~AreaMIDIAmbiencesRec() {
}

bool AreaMIDIAmbiencesRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempDaySequenceIndices[1];
  unsigned int tempNightSequenceIndices[1];
  unsigned int tempDLSFileIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, tempDaySequenceIndices, sizeof(tempDaySequenceIndices), 0, 0, 0) && result;
  result = SFile::Read(f, tempNightSequenceIndices, sizeof(tempNightSequenceIndices), 0, 0, 0) && result;
  result = SFile::Read(f, tempDLSFileIndices, sizeof(tempDLSFileIndices), 0, 0, 0) && result;
  result = SFile::Read(f, &m_volume, sizeof(m_volume), 0, 0, 0) && result;

  if (!result) {
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
