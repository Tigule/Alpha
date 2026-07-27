#include "ZoneMusicRec.h"

#include <Console/ConsoleClient.h>

namespace {
  const float PI = 3.14159265358979323846f;
  const float TWO_PI = PI + PI;
  const float OO_TWO_PI = 1.0f / TWO_PI;

}  // namespace

const char *ZoneMusicRec::GetFilename() {
  return "DBFilesClient\\ZoneMusic.dbc";
}

ZoneMusicRec::ZoneMusicRec() {
}

ZoneMusicRec::~ZoneMusicRec() {
}

bool ZoneMusicRec::Read(SFile *f, const char *stringBuffer) {
  bool         result = true;
  unsigned int tempMusicFileIndices[2];

  result = SFile::Read(f, &m_ID, sizeof(m_ID), 0, 0, 0) && result;
  result = SFile::Read(f, &m_VolumeFloat, sizeof(m_VolumeFloat), 0, 0, 0) && result;
  result = SFile::Read(f, &tempMusicFileIndices[0], sizeof(tempMusicFileIndices[0]), 0, 0, 0) && result;
  result = SFile::Read(f, &tempMusicFileIndices[1], sizeof(tempMusicFileIndices[1]), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SilenceIntervalMin[0], sizeof(m_SilenceIntervalMin), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SilenceIntervalMax[0], sizeof(m_SilenceIntervalMax), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SegmentLength[0], sizeof(m_SegmentLength), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SegmentPlayMin[0], sizeof(m_SegmentPlayMin), 0, 0, 0) && result;
  result = SFile::Read(f, &m_SegmentPlayMax[0], sizeof(m_SegmentPlayMax), 0, 0, 0) && result;
  result = SFile::Read(f, &m_Sounds[0], sizeof(m_Sounds), 0, 0, 0) && result;

  if (!result) {
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
