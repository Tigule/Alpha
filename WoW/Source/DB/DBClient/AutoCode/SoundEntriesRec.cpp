#include "SoundEntriesRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *SoundEntriesRec::GetFilename() {
  return "DBFilesClient\\SoundEntries.dbc";
}

SoundEntriesRec::SoundEntriesRec() {
}

SoundEntriesRec::~SoundEntriesRec() {
}

bool SoundEntriesRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempnameIndices[1];
  unsigned int tempFileIndices[10];
  unsigned int tempDirectoryBaseIndices[1];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_soundType, sizeof(m_soundType), 0, 0, 0) && result;
  result = SFile::Read(f, &tempnameIndices[0], sizeof(tempnameIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempFileIndices[0], sizeof(tempFileIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempFileIndices[1], sizeof(tempFileIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempFileIndices[2], sizeof(tempFileIndices[2]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempFileIndices[3], sizeof(tempFileIndices[3]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempFileIndices[4], sizeof(tempFileIndices[4]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempFileIndices[5], sizeof(tempFileIndices[5]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempFileIndices[6], sizeof(tempFileIndices[6]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempFileIndices[7], sizeof(tempFileIndices[7]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempFileIndices[8], sizeof(tempFileIndices[8]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempFileIndices[9], sizeof(tempFileIndices[9]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Freq[0], sizeof(m_Freq), 0, 0, 0) && result;
  result = SFile::Read(f, &tempDirectoryBaseIndices[0], sizeof(tempDirectoryBaseIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_volumeFloat, sizeof(m_volumeFloat), 0, 0, 0) && result;
  result = SFile::Read(f, &m_pitch, sizeof(m_pitch), 0, 0, 0) && result;
  result = SFile::Read(f, &m_pitchVariation, sizeof(m_pitchVariation), 0, 0, 0) && result;
  result = SFile::Read(f, &m_priority, sizeof(m_priority), 0, 0, 0) && result;
  result = SFile::Read(f, &m_channel, sizeof(m_channel), 0, 0, 0) && result;
  result = SFile::Read(f, &m_flags, sizeof(m_flags), 0, 0, 0) && result;
  result = SFile::Read(f, &m_minDistance, sizeof(m_minDistance), 0, 0, 0) && result;
  result = SFile::Read(f, &m_maxDistance, sizeof(m_maxDistance), 0, 0, 0) && result;
  result = SFile::Read(f, &m_distanceCutoff, sizeof(m_distanceCutoff), 0, 0, 0) && result;
  result = SFile::Read(f, &m_EAXDef, sizeof(m_EAXDef), 0, 0, 0) && result;

  if (!result) {
    ConsoleWrite("Error reading SoundEntriesRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_name = stringBuffer + tempnameIndices[0];
    m_File[0] = stringBuffer + tempFileIndices[0];
    m_File[1] = stringBuffer + tempFileIndices[1];
    m_File[2] = stringBuffer + tempFileIndices[2];
    m_File[3] = stringBuffer + tempFileIndices[3];
    m_File[4] = stringBuffer + tempFileIndices[4];
    m_File[5] = stringBuffer + tempFileIndices[5];
    m_File[6] = stringBuffer + tempFileIndices[6];
    m_File[7] = stringBuffer + tempFileIndices[7];
    m_File[8] = stringBuffer + tempFileIndices[8];
    m_File[9] = stringBuffer + tempFileIndices[9];
    m_DirectoryBase = stringBuffer + tempDirectoryBaseIndices[0];
  } else {
    m_name = "";
    m_File[0] = "";
    m_File[1] = "";
    m_File[2] = "";
    m_File[3] = "";
    m_File[4] = "";
    m_File[5] = "";
    m_File[6] = "";
    m_File[7] = "";
    m_File[8] = "";
    m_File[9] = "";
    m_DirectoryBase = "";
  }

  return true;
}
