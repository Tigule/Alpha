#pragma once

#include "Os/Mac/LoaderThread.h"

#include <AudioToolbox/AudioToolbox.h>

struct CacheData;

#pragma pack(push, 1)
struct tWAVEFORMATEX {
  UInt16 wFormatTag;
  UInt16 nChannels;
  UInt32 nSamplesPerSec;
  UInt32 nAvgBytesPerSec;
  UInt16 nBlockAlign;
  UInt16 wBitsPerSample;
  UInt16 cbSize;
};
#pragma pack(pop)

namespace WAV {
unsigned char *find_chunk(unsigned char *ptr, unsigned char *end, const char *id, UInt32 *size);
unsigned char *sfx_find_data(unsigned char *header, UInt32 *size, tWAVEFORMATEX *format);
}

class WAVPlayer : public StreamPlayer {
 public:
  WAVPlayer();
  virtual ~WAVPlayer();
  virtual void        Start();
  virtual void        Stop();
  virtual void        Reset();
  virtual void        SetVolume(float volume);
  virtual bool        IsPlaying();
  virtual PlayerType  Identify();
  virtual const char *GetSource();
  virtual void        SetLoopCount(UINT loopCount);
  virtual void        Set3DPositionSpherical(float distance, float azimuth, float elevation);
  virtual void        ReadAhead();
  bool                SetWAVSource(const char *name);
  void                SetFrequency(UINT frequency);

 private:
  bool            SetWAVSourceImmediate(const char *name);
  void            StartImmediate();
  void            UpdateFormatConverterNode();
  void            DeleteFormatConverterNode();
  static OSStatus Callback(
      void *refCon, AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *timeStamp, UInt32 busNumber, UInt32 numberFrames,
      AudioBufferList *data
  );

  SCritSect     m_lock;
  char         *m_source;
  volatile bool m_startPending;
  volatile bool m_stopPending;
  volatile bool m_sourcePending;
  bool          m_sourceValid;
  UInt32        m_dataSize;
  UInt32        m_headerSize;
  UInt32        m_chunkSize;
  UInt32        m_totalChunks;
  volatile int  m_chunkIndex;
  int           m_loadedChunkIndex;
  UInt32        m_position;
  volatile UINT m_loopCount;
  volatile UINT m_frequency;
  LONG         *m_chunkCounts[4];
  tWAVEFORMATEX m_converterFormat;
  tWAVEFORMATEX m_fileFormat;
  AUNode        m_wavPlayerNode;
  AudioUnit     m_wavPlayerUnit;
  CacheData    *m_cacheItem;
  float         m_volume;
  volatile bool m_playing;
  bool          m_connected;
  float         m_distance;
  float         m_elevation;
  float         m_azimuth;
};
