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
  int  error = 0;

  error |= (SFileReadTyped(f, &m_ID) == 0);
  error |= (SFileReadTyped(f, &m_VolumeFloat) == 0);
  error |= (SFileReadTyped(f, &tempMusicFileIndices[0]) == 0);
  error |= (SFileReadTyped(f, &tempMusicFileIndices[1]) == 0);
  error |= (SFileReadTyped(f, &m_SilenceIntervalMin) == 0);
  error |= (SFileReadTyped(f, &m_SilenceIntervalMax) == 0);
  error |= (SFileReadTyped(f, &m_SegmentLength) == 0);
  error |= (SFileReadTyped(f, &m_SegmentPlayMin) == 0);
  error |= (SFileReadTyped(f, &m_SegmentPlayMax) == 0);
  error |= (SFileReadTyped(f, &m_Sounds) == 0);

  if (error) {
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
