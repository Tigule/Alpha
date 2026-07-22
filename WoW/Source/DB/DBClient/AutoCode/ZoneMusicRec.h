#pragma once

#include <DB/WowClientDB.h>

class ZoneMusicRec {
 public:
  ZoneMusicRec();
  ~ZoneMusicRec();

  static const char *__fastcall GetFilename();

  static unsigned int GetNumColumns() {
    return 16;
  }

  static unsigned int GetRowSize() {
    return 64;
  }

  int GetID() {
    return m_ID;
  }

  bool NeedIDAssigned() {
    return false;
  }

  void SetID(int id) {
  }

  bool Read(SFile *f, const char *stringBuffer);

  int         m_ID;
  float       m_VolumeFloat;
  const char *m_MusicFile[2];
  int         m_SilenceIntervalMin[2];
  int         m_SilenceIntervalMax[2];
  int         m_SegmentLength[2];
  int         m_SegmentPlayMin[2];
  int         m_SegmentPlayMax[2];
  int         m_Sounds[2];
};

extern WowClientDB<ZoneMusicRec> g_zoneMusicDB;
