#include <Base/Base.h>

#include <storm.h>

#include "Os/Mac/GlobalAUGraph.h"
#include "Os/Mac/MP3Player.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void Console(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
}

MP3Player::MP3Player()
    : m_data(0),
      m_size(0),
      m_file(0),
      m_movie(0),
      m_packetsPerBuffer(0),
      m_packetIndex(0),
      m_loopCount(1),
      m_loopsRemaining(1),
      m_running(false),
      m_ended(false),
      m_volume(1.0f) {
  m_name[0] = 0;
  memset(m_buffers, 0, sizeof(m_buffers));
}

MP3Player::~MP3Player() {
  Reset();
}

Player::PlayerType MP3Player::Identify() {
  return e_mp3Player;
}

void MP3Player::ReadAhead() {
}

const char *MP3Player::GetSource() {
  return m_name;
}

void MP3Player::SetLoopCount(UINT loopCount) {
  m_loopCount = loopCount;
}

bool MP3Player::IsPlaying() {
  return m_movie && (m_running || !m_ended);
}

void MP3Player::SetVolume(float volume) {
  m_volume = volume;

  if (m_movie) {
    AudioQueueSetParameter(m_movie, kAudioQueueParam_Volume, m_volume);
  }
}

void MP3Player::Stop() {
  if (m_movie) {
    m_running = false;
    AudioQueueStop(m_movie, true);
  }
}

void MP3Player::Start() {
  if (!m_movie) {
    return;
  }

  AudioQueueStop(m_movie, true);

  m_packetIndex = 0;
  m_loopsRemaining = m_loopCount;
  m_ended = false;
  m_running = true;

  for (int i = 0; i < NUM_BUFFERS; ++i) {
    OutputCallback(this, m_movie, m_buffers[i]);
  }

  AudioQueueSetParameter(m_movie, kAudioQueueParam_Volume, m_volume);

  if (AudioQueueStart(m_movie, 0)) {
    m_running = false;
  }
}

void MP3Player::Reset() {
  if (m_movie) {
    AudioQueueDispose(m_movie, true);
    m_movie = 0;
    memset(m_buffers, 0, sizeof(m_buffers));
  }

  if (m_file) {
    AudioFileClose(m_file);
    m_file = 0;
  }

  if (m_data) {
    SMemFree(m_data, __FILE__, __LINE__, 0);
    m_data = 0;
  }

  m_size = 0;
  m_name[0] = 0;
  m_running = false;
  m_ended = false;
  m_volume = 1.0f;
}

bool MP3Player::SetMP3Source(const char *name) {
  OSStatus err;
  HSFILE   file;

  Reset();

  if (!SFileOpenFile(name, &file)) {
    return false;
  }

  m_size = SFileGetFileSize(file, 0);
  m_data = SMemAlloc(m_size, __FILE__, __LINE__, 0);

  DWORD actualRead = 0;
  BOOL  success = SFileReadFile(file, m_data, m_size, &actualRead, 0);
  SFileCloseFile(file);

  if (!success || actualRead != m_size) {
    Reset();
    return false;
  }

  AudioStreamBasicDescription format;
  UInt32                      size = sizeof(format);
  UInt32                      maxPacketSize = 0;
  err = AudioFileOpenWithCallbacks(this, ReadProc, 0, GetSizeProc, 0, kAudioFileMP3Type, &m_file);
  if (err) {
    m_file = 0;
  } else {
    err = AudioFileGetProperty(m_file, kAudioFilePropertyDataFormat, &size, &format);
  }
  if (!err) {
    size = sizeof(maxPacketSize);
    err = AudioFileGetProperty(m_file, kAudioFilePropertyPacketSizeUpperBound, &size, &maxPacketSize);
  }
  if (err) {
    Console("Couldn't MovieImportDataRef (%s, %d bytes, %d).", name, (int)m_size, (int)err);
    Reset();
    return false;
  }

  err = AudioQueueNewOutput(&format, OutputCallback, this, 0, 0, 0, &m_movie);
  if (err) {
    SErrPrepareAppFatal(__FILE__, __LINE__);
    SErrDisplayAppFatal("Couldn't instance MP3 component (%d).", (int)err);
  }

  ASSERT(m_movie);

  m_packetsPerBuffer = BUFFER_SIZE / (maxPacketSize ? maxPacketSize : 1);
  if (!m_packetsPerBuffer) {
    m_packetsPerBuffer = 1;
  }

  for (int i = 0; i < NUM_BUFFERS; ++i) {
    AudioQueueAllocateBufferWithPacketDescriptions(m_movie, m_packetsPerBuffer * maxPacketSize, m_packetsPerBuffer, &m_buffers[i]);
  }

  AudioQueueAddPropertyListener(m_movie, kAudioQueueProperty_IsRunning, RunningCallback, this);

  SStrCopy(m_name, name, sizeof(m_name));
  return true;
}

OSStatus MP3Player::ReadProc(void *clientData, SInt64 position, UInt32 requestCount, void *buffer, UInt32 *actualCount) {
  MP3Player *player = (MP3Player *)clientData;

  if (position >= player->m_size) {
    *actualCount = 0;
    return noErr;
  }

  if (requestCount > player->m_size - position) {
    requestCount = player->m_size - position;
  }

  memcpy(buffer, (char *)player->m_data + position, requestCount);
  *actualCount = requestCount;
  return noErr;
}

SInt64 MP3Player::GetSizeProc(void *clientData) {
  return ((MP3Player *)clientData)->m_size;
}

void MP3Player::OutputCallback(void *userData, AudioQueueRef queue, AudioQueueBufferRef buffer) {
  MP3Player *player = (MP3Player *)userData;

  if (player->m_ended || !buffer) {
    return;
  }

  UInt32 bytes = buffer->mAudioDataBytesCapacity;
  UInt32 packets = player->m_packetsPerBuffer;
  AudioFileReadPacketData(player->m_file, false, &bytes, buffer->mPacketDescriptions, player->m_packetIndex, &packets, buffer->mAudioData);

  if (!packets && player->m_loopsRemaining != 1) {
    if (player->m_loopsRemaining) {
      --player->m_loopsRemaining;
    }
    player->m_packetIndex = 0;
    bytes = buffer->mAudioDataBytesCapacity;
    packets = player->m_packetsPerBuffer;
    AudioFileReadPacketData(player->m_file, false, &bytes, buffer->mPacketDescriptions, player->m_packetIndex, &packets, buffer->mAudioData);
  }

  if (!packets) {
    player->m_ended = true;
    AudioQueueStop(queue, false);
    return;
  }

  buffer->mAudioDataByteSize = bytes;
  buffer->mPacketDescriptionCount = packets;
  player->m_packetIndex += packets;
  AudioQueueEnqueueBuffer(queue, buffer, 0, 0);
}

void MP3Player::RunningCallback(void *userData, AudioQueueRef queue, AudioQueuePropertyID property) {
  MP3Player *player = (MP3Player *)userData;
  UInt32     running = 0;
  UInt32     size = sizeof(running);

  AudioQueueGetProperty(queue, kAudioQueueProperty_IsRunning, &running, &size);
  if (!running && player->m_ended) {
    player->m_running = false;
  }
}
