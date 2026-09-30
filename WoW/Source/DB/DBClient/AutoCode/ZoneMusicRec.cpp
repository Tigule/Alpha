#include "ZoneMusicRec.h"

#include <Base/Base.h>
#include <Console/ConsoleClient.h>

LPCSTR ZoneMusicRec::GetFilename() {
  return "DBFilesClient\\ZoneMusic.dbc";
}

ZoneMusicRec::ZoneMusicRec() {
}

ZoneMusicRec::~ZoneMusicRec() {
}

bool ZoneMusicRec::Read(SFile *f, LPCSTR stringBuffer) {
  UINT tempMusicFileIndices[2];

  if (!SFileReadTyped(f, &m_ID) ||
      !SFileReadTyped(f, &m_VolumeFloat) ||
      !SFileReadTyped(f, &tempMusicFileIndices[0]) ||
      !SFileReadTyped(f, &tempMusicFileIndices[1]) ||
      !SFile::Read(f, &m_SilenceIntervalMin[0], sizeof(m_SilenceIntervalMin), 0, 0, 0) ||
      !SFile::Read(f, &m_SilenceIntervalMax[0], sizeof(m_SilenceIntervalMax), 0, 0, 0) ||
      !SFile::Read(f, &m_SegmentLength[0], sizeof(m_SegmentLength), 0, 0, 0) ||
      !SFile::Read(f, &m_SegmentPlayMin[0], sizeof(m_SegmentPlayMin), 0, 0, 0) ||
      !SFile::Read(f, &m_SegmentPlayMax[0], sizeof(m_SegmentPlayMax), 0, 0, 0) ||
      !SFile::Read(f, &m_Sounds[0], sizeof(m_Sounds), 0, 0, 0)) {
    ConsoleWrite("Error reading ZoneMusicRec", DEFAULT_COLOR);
    return false;
  }

  if (stringBuffer) {
    m_MusicFile[0] = stringBuffer + tempMusicFileIndices[0];
    m_MusicFile[1] = stringBuffer + tempMusicFileIndices[1];
  } else {
    m_MusicFile[0] = "";
    m_MusicFile[1] = "";
  }

  return true;
}
