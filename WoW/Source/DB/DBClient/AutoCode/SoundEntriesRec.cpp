#include "SoundEntriesRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR SoundEntriesRec::GetFilename() {
  return "DBFilesClient\\SoundEntries.dbc";
}

SoundEntriesRec::SoundEntriesRec() {
}

SoundEntriesRec::~SoundEntriesRec() {
}

bool SoundEntriesRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempnameIndices[1];
  UINT tempFileIndices[10];
  UINT tempDirectoryBaseIndices[1];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_soundType) ||
      !SFileReadTyped(f, &tempnameIndices[0]) ||
      !SFileReadTyped(f, &tempFileIndices[0]) ||
      !SFileReadTyped(f, &tempFileIndices[1]) ||
      !SFileReadTyped(f, &tempFileIndices[2]) ||
      !SFileReadTyped(f, &tempFileIndices[3]) ||
      !SFileReadTyped(f, &tempFileIndices[4]) ||
      !SFileReadTyped(f, &tempFileIndices[5]) ||
      !SFileReadTyped(f, &tempFileIndices[6]) ||
      !SFileReadTyped(f, &tempFileIndices[7]) ||
      !SFileReadTyped(f, &tempFileIndices[8]) ||
      !SFileReadTyped(f, &tempFileIndices[9]) ||
      !SFile::Read(f, &m_Freq[0], sizeof(m_Freq), 0, 0, 0) ||
      !SFileReadTyped(f, &tempDirectoryBaseIndices[0]) ||
      !SFileReadTyped(f, &m_volumeFloat) ||
      !SFileReadTyped(f, &m_pitch) ||
      !SFileReadTyped(f, &m_pitchVariation) ||
      !SFileReadTyped(f, &m_priority) ||
      !SFileReadTyped(f, &m_channel) ||
      !SFileReadTyped(f, &m_flags) ||
      !SFileReadTyped(f, &m_minDistance) ||
      !SFileReadTyped(f, &m_maxDistance) ||
      !SFileReadTyped(f, &m_distanceCutoff) ||
      !SFileReadTyped(f, &m_EAXDef)) {
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
