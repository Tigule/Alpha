#pragma once

#include "Os/Mac/LoaderThread.h"

#include <AudioToolbox/AudioToolbox.h>

class MP3Player : public StreamPlayer {
 public:
  enum {
    NUM_BUFFERS = 3,
    BUFFER_SIZE = 0x8000
  };

  MP3Player();
  virtual ~MP3Player();
  virtual void        Start();
  virtual void        Stop();
  virtual void        Reset();
  virtual void        SetVolume(float volume);
  virtual bool        IsPlaying();
  virtual PlayerType  Identify();
  virtual const char *GetSource();
  virtual void        SetLoopCount(UINT loopCount);
  virtual void        ReadAhead();
  bool                SetMP3Source(const char *name);

 private:
  static OSStatus ReadProc(void *clientData, SInt64 position, UInt32 requestCount, void *buffer, UInt32 *actualCount);
  static SInt64   GetSizeProc(void *clientData);
  static void     OutputCallback(void *userData, AudioQueueRef queue, AudioQueueBufferRef buffer);
  static void     RunningCallback(void *userData, AudioQueueRef queue, AudioQueuePropertyID property);

  char                          m_name[260];
  void                         *m_data;
  UInt32                        m_size;
  AudioFileID                   m_file;
  AudioQueueRef                 m_movie;
  AudioQueueBufferRef           m_buffers[NUM_BUFFERS];
  UInt32                        m_packetsPerBuffer;
  SInt64                        m_packetIndex;
  volatile UINT                 m_loopCount;
  UINT                          m_loopsRemaining;
  volatile bool                 m_running;
  volatile bool                 m_ended;
  float                         m_volume;
};
