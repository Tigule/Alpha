#include <Base/Base.h>

#include <storm.h>

#include "Os/Mac/GlobalAUGraph.h"
#include "Os/Mac/StreamingCache.h"
#include "Os/Mac/WAVPlayer.h"

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

unsigned char *WAV::find_chunk(unsigned char *ptr, unsigned char *end, const char *id, UInt32 *size) {
  while (ptr + 8 <= end) {
    UInt32 chunkSize;
    memcpy(&chunkSize, ptr + 4, sizeof(chunkSize));
    if (!memcmp(ptr, id, 4)) {
      *size = chunkSize;
      return ptr + 8;
    }
    ptr += 8 + chunkSize;
  }
  return 0;
}

unsigned char *WAV::sfx_find_data(unsigned char *header, UInt32 *size, tWAVEFORMATEX *format) {
  unsigned char *end = header + *size;
  UInt32         chunkSize;

  if (*size < 12 || memcmp(header, "RIFF", 4) || memcmp(header + 8, "WAVE", 4)) {
    return 0;
  }

  unsigned char *fmt = find_chunk(header + 12, end, "fmt ", &chunkSize);
  if (!fmt || chunkSize < 16 || fmt + 16 > end) {
    return 0;
  }

  memset(format, 0, sizeof(*format));
  memcpy(format, fmt, chunkSize < sizeof(*format) ? chunkSize : sizeof(*format));

  unsigned char *data = find_chunk(header + 12, end, "data", &chunkSize);
  if (!data) {
    return 0;
  }

  *size = chunkSize;
  return data;
}

WAVPlayer::WAVPlayer()
    : m_source(0),
      m_startPending(false),
      m_stopPending(false),
      m_sourcePending(false),
      m_sourceValid(false),
      m_dataSize(0),
      m_headerSize(0),
      m_chunkSize(0),
      m_totalChunks(0),
      m_chunkIndex(-1),
      m_loadedChunkIndex(-1),
      m_position(0),
      m_loopCount(1),
      m_frequency(0),
      m_wavPlayerNode(0),
      m_wavPlayerUnit(0),
      m_cacheItem(0),
      m_volume(1.0f),
      m_playing(false),
      m_connected(false),
      m_distance(1.0f),
      m_elevation(0.0f),
      m_azimuth(0.0f) {
  memset(m_chunkCounts, 0, sizeof(m_chunkCounts));
  memset(&m_converterFormat, 0, sizeof(m_converterFormat));
  memset(&m_fileFormat, 0, sizeof(m_fileFormat));
}

WAVPlayer::~WAVPlayer() {
  Reset();
  DeleteFormatConverterNode();
}

Player::PlayerType WAVPlayer::Identify() {
  return e_wavPlayer;
}

const char *WAVPlayer::GetSource() {
  return m_cacheItem ? m_cacheItem->GetString() : "";
}

void WAVPlayer::SetFrequency(UINT frequency) {
  m_frequency = frequency;
}

void WAVPlayer::SetLoopCount(UINT loopCount) {
  m_loopCount = loopCount;
}

bool WAVPlayer::IsPlaying() {
  return m_startPending || m_sourcePending || (((UINT)m_chunkIndex < m_totalChunks || m_loopCount != 1) && m_playing);
}

void WAVPlayer::Reset() {
  m_lock.Enter();

  SetStreamState(e_inactive);

  if (m_wavPlayerNode && GlobalAUGraph::GetAUGraph()) {
    GlobalAUGraph::Get().m_mixer.LabelNode(m_wavPlayerNode, GetSource(), "STOP");
    m_playing = false;
    m_chunkIndex = -1;
  }

  if (m_source) {
    SMemFree(m_source, __FILE__, __LINE__, 0);
    m_source = 0;
  }

  m_startPending = false;
  m_stopPending = false;

  if (g_streamingCache) {
    if (m_cacheItem) {
      g_streamingCache->Release(m_cacheItem);
    }

    for (int i = 0; i < 4; ++i) {
      if (m_chunkCounts[i]) {
        CheckIn(m_chunkCounts[i]);
      }
    }
  }

  m_cacheItem = 0;
  m_loadedChunkIndex = -1;
  memset(m_chunkCounts, 0, sizeof(m_chunkCounts));

  m_dataSize = 0;
  m_headerSize = 0;
  m_chunkSize = 0;
  m_totalChunks = 0;
  m_chunkIndex = -1;
  m_frequency = 0;
  m_sourceValid = false;
  m_volume = 1.0f;
  m_distance = 1.0f;
  m_elevation = 0.0f;
  m_azimuth = 0.0f;

  m_lock.Leave();
}

bool WAVPlayer::SetWAVSource(const char *name) {
  Reset();
  m_sourcePending = true;
  m_source = SStrDupA(name, __FILE__, __LINE__);
  SetStreamState(e_active);
  return true;
}

void WAVPlayer::Start() {
  m_stopPending = false;
  m_startPending = true;
  SetStreamState(e_active);
}

void WAVPlayer::Stop() {
  m_startPending = false;
  m_stopPending = true;
  SetStreamState(e_active);
}

bool WAVPlayer::SetWAVSourceImmediate(const char *name) {
  void *header;

  m_cacheItem = g_streamingCache->Acquire(name, &m_headerSize, &header, 1024);
  if (!m_cacheItem) {
    Console(name);
    memset(&m_fileFormat, 0, sizeof(m_fileFormat));
    return false;
  }

  m_dataSize = m_headerSize;
  unsigned char *data = WAV::sfx_find_data((unsigned char *)header, &m_dataSize, &m_fileFormat);
  UInt32         dataOffset = data ? (UInt32)(data - (unsigned char *)header) : 0;

  if (!data || !m_dataSize || (m_fileFormat.wBitsPerSample != 8 && m_fileFormat.wBitsPerSample != 16) || !dataOffset || dataOffset >= m_headerSize) {
    Console("WAVPlayer::SetWAVSourceImmediate: [%s] could not be parsed; probably it's corrupt.", name);
    g_streamingCache->Release(m_cacheItem);
    m_cacheItem = 0;
    return false;
  }

  if (m_fileFormat.wBitsPerSample != 8 && m_fileFormat.wBitsPerSample != 16) {
    Console("WAVPlayer::SetWAVSourceImmediate: [%s] says it is %d-bit, which is preposterous.", name, m_fileFormat.wBitsPerSample);
    g_streamingCache->Release(m_cacheItem);
    m_cacheItem = 0;
    return false;
  }

  if (m_fileFormat.nChannels != 1 && m_fileFormat.nChannels != 2) {
    Console("WAVPlayer::SetWAVSourceImmediate: [%s] says it is %d-channel, which we don't support.", name, m_fileFormat.nChannels);
    g_streamingCache->Release(m_cacheItem);
    m_cacheItem = 0;
    return false;
  }

  m_chunkSize = m_fileFormat.nChannels * (m_fileFormat.wBitsPerSample / 8) * (m_fileFormat.nSamplesPerSec * 250 / 1000);
  if (!m_frequency) {
    m_frequency = m_fileFormat.nSamplesPerSec;
  }

  g_streamingCache->SetCacheParameters(m_cacheItem, dataOffset, m_dataSize, m_chunkSize);
  m_totalChunks = g_streamingCache->GetTotalChunks(m_cacheItem);
  m_chunkIndex = 0;
  m_sourceValid = true;
  return true;
}

void WAVPlayer::ReadAhead() {
  bool locked = false;

  if (m_source && (locked || (locked = m_lock.TryEnter() != 0))) {
    SetWAVSourceImmediate(m_source);
    SMemFree(m_source, __FILE__, __LINE__, 0);
    m_source = 0;
    m_sourcePending = false;
  }

  if (m_loadedChunkIndex != m_chunkIndex && (locked || (locked = m_lock.TryEnter() != 0))) {
    m_loadedChunkIndex = m_chunkIndex;

    for (int i = 0; i < 4; ++i) {
      if (m_chunkCounts[i]) {
        LONG *count = m_chunkCounts[i];
        m_chunkCounts[i] = 0;
        CheckIn(count);
      }
    }

    if (m_loadedChunkIndex != -1 && m_cacheItem) {
      for (int i = 0; i < 4; ++i) {
        UINT chunk = m_loadedChunkIndex + i;
        if (m_loopCount != 1 && chunk >= m_totalChunks) {
          chunk %= m_totalChunks;
        }
        if (chunk < m_totalChunks) {
          g_streamingCache->Load(m_cacheItem, chunk, &m_chunkCounts[i]);
        }
      }
    }
  }

  if (m_startPending && (locked || (locked = m_lock.TryEnter() != 0))) {
    StartImmediate();
    m_startPending = false;
  }

  if (m_stopPending && (locked || (locked = m_lock.TryEnter() != 0))) {
    if (m_wavPlayerNode && GlobalAUGraph::GetAUGraph()) {
      GlobalAUGraph::Get().m_mixer.LabelNode(m_wavPlayerNode, GetSource(), "STOP");
      m_playing = false;
      m_chunkIndex = -1;
    }
    m_stopPending = false;
  }

  if (locked) {
    m_lock.Leave();
  }
}

void WAVPlayer::StartImmediate() {
  OSStatus err;

  if (!m_sourceValid) {
    return;
  }

  if (!m_wavPlayerNode || !m_wavPlayerUnit || memcmp(&m_fileFormat, &m_converterFormat, sizeof(tWAVEFORMATEX))) {
    DeleteFormatConverterNode();
    m_converterFormat = m_fileFormat;
    UpdateFormatConverterNode();
  }

  m_chunkIndex = 0;
  m_position = 0;

  if (!m_wavPlayerNode || m_playing || !GlobalAUGraph::GetAUGraph()) {
    return;
  }

  Mixer &mixer = GlobalAUGraph::Get().m_mixer;

  if (!m_connected) {
    err = mixer.Connect(m_wavPlayerNode, 0);
    if (err == kAUGraphErr_InvalidConnection) {
      Console("WAVPlayer::Start: Global mixer has no unused inputs");
      mixer.Dump(Mixer::e_dumpVerbose);
      return;
    }
    ASSERTVALUE(err == noErr, err);
  }

  m_playing = true;
  mixer.LabelNode(m_wavPlayerNode, GetSource(), "PLAY");
  mixer.Dump(Mixer::e_dumpQuiet);

  err = mixer.SetMixVolume(m_wavPlayerNode, m_volume);
  ASSERTVALUE(err == noErr, err);

  err = mixer.Set3DDistance(m_wavPlayerNode, m_distance);
  ASSERTVALUE(err == noErr, err);

  err = mixer.Set3DAzimuth(m_wavPlayerNode, m_azimuth);
  ASSERTVALUE(err == noErr, err);

  err = mixer.Set3DElevation(m_wavPlayerNode, m_elevation);
  ASSERTVALUE(err == noErr, err);

  if (!m_connected) {
    err = AUGraphUpdateStable(GlobalAUGraph::GetAUGraph());
    ASSERTVALUE(err == noErr, err);
    m_connected = true;
  }
}

void WAVPlayer::DeleteFormatConverterNode() {
  OSStatus err;

  if (m_wavPlayerNode && GlobalAUGraph::GetAUGraph()) {
    if (m_connected) {
      err = GlobalAUGraph::Get().m_mixer.Disconnect(m_wavPlayerNode);
      ASSERTVALUE(err == noErr, err);

      err = AUGraphUpdateStable(GlobalAUGraph::GetAUGraph());
      ASSERTVALUE(err == noErr, err);


      m_connected = false;
    }

    err = AUGraphRemoveNode(GlobalAUGraph::GetAUGraph(), m_wavPlayerNode);
    ASSERTVALUE(err == noErr, err);

    m_wavPlayerNode = 0;
    m_wavPlayerUnit = 0;
    memset(&m_converterFormat, 0, sizeof(m_converterFormat));
  }
}

void WAVPlayer::UpdateFormatConverterNode() {
  OSStatus                    err;
  AudioComponentDescription   desc;
  AURenderCallbackStruct      callback;
  AudioStreamBasicDescription format;
  UInt32                      bytesPerFrame = m_converterFormat.nChannels * (m_converterFormat.wBitsPerSample / 8);

  ASSERT(0 == (m_chunkSize % bytesPerFrame));

  desc.componentType = kAudioUnitType_FormatConverter;
  desc.componentSubType = kAudioUnitSubType_AUConverter;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;
  err = AUGraphAddNode(GlobalAUGraph::GetAUGraph(), &desc, &m_wavPlayerNode);
  ASSERTVALUE(err == noErr, err);
  ASSERT(m_wavPlayerNode != 0);

  err = AUGraphNodeInfo(GlobalAUGraph::GetAUGraph(), m_wavPlayerNode, 0, &m_wavPlayerUnit);
  ASSERTVALUE(err == noErr, err);

  callback.inputProc = Callback;
  callback.inputProcRefCon = this;
  err = AudioUnitSetProperty(m_wavPlayerUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global, 0, &callback, sizeof(callback));
  ASSERTVALUE(err == noErr, err);

  format.mSampleRate = m_converterFormat.nSamplesPerSec;
  format.mFormatID = kAudioFormatLinearPCM;
  format.mFormatFlags = m_converterFormat.wBitsPerSample == 8 ? kLinearPCMFormatFlagIsPacked : kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
  format.mBytesPerPacket = bytesPerFrame;
  format.mFramesPerPacket = 1;
  format.mBytesPerFrame = bytesPerFrame;
  format.mChannelsPerFrame = m_converterFormat.nChannels;
  format.mBitsPerChannel = m_converterFormat.wBitsPerSample;
  format.mReserved = 0;
  err = AudioUnitSetProperty(m_wavPlayerUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(format));
  ASSERTVALUE(err == noErr, err);

  format.mSampleRate = 44100.0;
  format.mFormatID = kAudioFormatLinearPCM;
  format.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
  format.mBytesPerPacket = 4;
  format.mFramesPerPacket = 1;
  format.mBytesPerFrame = 4;
  format.mChannelsPerFrame = m_converterFormat.nChannels;
  format.mBitsPerChannel = 32;
  format.mReserved = 0;
  err = AudioUnitSetProperty(m_wavPlayerUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &format, sizeof(format));
  ASSERTVALUE(err == noErr, err);

  err = AudioUnitInitialize(m_wavPlayerUnit);
  ASSERTVALUE(err == noErr, err);
}

void WAVPlayer::SetVolume(float volume) {
  m_volume = volume;

  if (m_playing) {
    OSStatus err = GlobalAUGraph::Get().m_mixer.SetMixVolume(m_wavPlayerNode, m_volume);
    ASSERTVALUE(err == noErr, err);
  }
}

void WAVPlayer::Set3DPositionSpherical(float distance, float azimuth, float elevation) {
  OSStatus err;

  m_distance = 0.9144f * distance;
  if (m_distance < 0.01f) {
    m_distance = 0.01f;
  }

  m_elevation = 90.0f - elevation * 57.2957763671875f;

  float azimuthDegrees = azimuth * 57.2957763671875f;
  m_azimuth = azimuthDegrees > 0.0f ? 360.0f - azimuthDegrees : -azimuthDegrees;

  if (m_playing) {
    Mixer &mixer = GlobalAUGraph::Get().m_mixer;

    err = mixer.Set3DDistance(m_wavPlayerNode, m_distance);
    ASSERTVALUE(err == noErr, err);

    err = mixer.Set3DAzimuth(m_wavPlayerNode, m_azimuth);
    ASSERTVALUE(err == noErr, err);

    err = mixer.Set3DElevation(m_wavPlayerNode, m_elevation);
    ASSERTVALUE(err == noErr, err);
  }
}

OSStatus WAVPlayer::Callback(
    void *refCon, AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *timeStamp, UInt32 busNumber, UInt32 numberFrames,
    AudioBufferList *data
) {
  WAVPlayer     *player = (WAVPlayer *)refCon;
  unsigned char *out = (unsigned char *)data->mBuffers[0].mData;
  UInt32         bytesPerFrame = player->m_converterFormat.nBlockAlign;
  int            silence = player->m_converterFormat.wBitsPerSample == 8 ? 0x80 : 0;
  CacheData     *cacheItem = player->m_cacheItem;

  if (bytesPerFrame != 1 && bytesPerFrame != 2 && bytesPerFrame != 4) {
    bytesPerFrame = player->m_converterFormat.nChannels * (player->m_converterFormat.wBitsPerSample / 8);
  }

  UInt32 bytes = numberFrames * bytesPerFrame;
  if (bytes > data->mBuffers[0].mDataByteSize) {
    bytes = data->mBuffers[0].mDataByteSize;
  }

  if (!player->m_playing || !cacheItem || !player->m_chunkSize) {
    memset(out, silence, bytes);
    *actionFlags |= kAudioUnitRenderAction_OutputIsSilence;
    return noErr;
  }

  UInt32 written = 0;
  while (written < bytes) {
    if ((UINT)player->m_chunkIndex >= player->m_totalChunks) {
      if (player->m_loopCount == 1) {
        GlobalAUGraph::Get().m_mixer.LabelNode(player->m_wavPlayerNode, player->GetSource(), "OVER");
        player->m_playing = false;
        player->m_chunkIndex = -1;
        break;
      }
      GlobalAUGraph::Get().m_mixer.LabelNode(player->m_wavPlayerNode, player->GetSource(), "LOOP");
      player->m_position = 0;
      player->m_chunkIndex = 0;
      if (player->m_loopCount) {
        --player->m_loopCount;
      }
    }

    UInt32 chunk = player->m_position / player->m_chunkSize;
    UInt32 offset = player->m_position % player->m_chunkSize;
    void  *chunkData;
    UINT   chunkBytes;
    LONG  *count;

    if (!g_streamingCache->Stream(cacheItem, chunk, &chunkData, &chunkBytes, &count)) {
      break;
    }

    if (player->m_frequency == player->m_converterFormat.nSamplesPerSec) {
      UInt32 copy = chunkBytes - offset;
      if (copy > bytes - written) {
        copy = bytes - written;
      }
      copy -= copy % bytesPerFrame;
      memcpy(out + written, (unsigned char *)chunkData + offset, copy);
      written += copy;
      player->m_position += copy;
    } else {
      UInt32 step = (UInt32)(((UInt64)player->m_frequency << 16) / player->m_converterFormat.nSamplesPerSec);
      UInt32 fraction = 0;
      while (written < bytes && offset + bytesPerFrame <= chunkBytes) {
        memcpy(out + written, (unsigned char *)chunkData + offset, bytesPerFrame);
        written += bytesPerFrame;
        fraction += step;
        UInt32 advance = (fraction >> 16) * bytesPerFrame;
        fraction &= 0xFFFF;
        offset += advance;
        player->m_position += advance;
      }
    }

    CheckIn(count);

    if (player->m_position >= player->m_dataSize ||
        (chunkBytes < player->m_chunkSize && player->m_position % player->m_chunkSize + bytesPerFrame > chunkBytes)) {
      player->m_chunkIndex = player->m_totalChunks;
    } else {
      player->m_chunkIndex = player->m_position / player->m_chunkSize;
    }
  }

  if (written < bytes) {
    memset(out + written, silence, bytes - written);
  }

  return noErr;
}
