#include <Base/Base.h>

#include <storm.h>

#include "Os/Mac/GlobalAUGraph.h"
#include "Os/Mac/MIDIPlayer.h"
#include "Os/OsTime.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

void OsPathGetFilename(LPCSTR path, char *filename, UINT filenameSize);
bool GetTemporaryFSSpec(const char *name, char *path, UINT pathSize);

static void Console(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
}

static CFURLRef CreateTemporaryFromMPQ(const char *name) {
  HSFILE file;
  if (!SFileOpenFile(name, &file)) {
    return 0;
  }

  DWORD hash = SStrHash(name, 0, 0);
  DWORD size = SFileGetFileSize(file, 0);

  char filename[260];
  OsPathGetFilename(name, filename, sizeof(filename));

  char tempName[260];
  SStrPrintf(tempName, sizeof(tempName), "%x.%s", (UINT)hash, filename);

  char path[1024];
  if (!GetTemporaryFSSpec(tempName, path, sizeof(path))) {
    SFileCloseFile(file);
    return 0;
  }

  struct stat info;
  if (stat(path, &info) || (DWORD)info.st_size != size) {
    void *data = SMemAlloc(size, __FILE__, __LINE__, 0);
    DWORD actualRead = 0;
    if (!SFileReadFile(file, data, size, &actualRead, 0) || actualRead != size) {
      SMemFree(data, __FILE__, __LINE__, 0);
      SFileCloseFile(file);
      return 0;
    }

    FILE *out = fopen(path, "wb");
    if (!out) {
      SMemFree(data, __FILE__, __LINE__, 0);
      SFileCloseFile(file);
      return 0;
    }
    fwrite(data, 1, size, out);
    fclose(out);

    SMemFree(data, __FILE__, __LINE__, 0);
  }

  SFileCloseFile(file);

  return CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)path, SStrLen(path), false);
}

static OSStatus MusicSequenceSetDestNode(MusicSequence sequence, AUNode node, AUGraph graph) {
  OSStatus err;
  UInt32   trackCount;
  MusicTimeStamp sequenceLength = 0;

  err = MusicSequenceSetAUGraph(sequence, graph);
  if (err) {
    return err;
  }

  err = MusicSequenceGetTrackCount(sequence, &trackCount);
  if (err) {
    return err;
  }

  for (UInt32 i = 0; i < trackCount; ++i) {
    MusicTrack     track;
    MusicTimeStamp trackLength;
    UInt32         lengthSize = sizeof(trackLength);
    err = MusicSequenceGetIndTrack(sequence, i, &track);
    if (err) {
      return err;
    }
    err = MusicTrackSetDestNode(track, node);
    if (err) {
      return err;
    }
    if (!MusicTrackGetProperty(track, kSequenceTrackProperty_TrackLength, &trackLength, &lengthSize) && trackLength > sequenceLength) {
      sequenceLength = trackLength;
    }
  }

  for (UInt32 i = 0; i < trackCount; ++i) {
    MusicTrack         track;
    MusicTrackLoopInfo loopInfo;
    loopInfo.loopDuration = sequenceLength;
    loopInfo.numberOfLoops = 0;
    if (sequenceLength > 0.0 && !MusicSequenceGetIndTrack(sequence, i, &track)) {
      MusicTrackSetProperty(track, kSequenceTrackProperty_LoopInfo, &loopInfo, sizeof(loopInfo));
    }
  }

  return noErr;
}

MIDIPlayer::MIDIPlayer()
    : m_dlsURL(0),
      m_midiURL(0),
      m_synthNode(0),
      m_synthUnit(0),
      m_sequence(0),
      m_player(0),
      m_startTicks(0),
      m_volume(1.0f),
      m_connected(false) {
  m_midiName[0] = 0;
  m_dlsName[0] = 0;
}

MIDIPlayer::~MIDIPlayer() {
  Reset();
}

Player::PlayerType MIDIPlayer::Identify() {
  return e_midiPlayer;
}

void MIDIPlayer::ReadAhead() {
}

const char *MIDIPlayer::GetSource() {
  return m_midiName;
}

const char *MIDIPlayer::GetSourceDLS() {
  return m_dlsName;
}

void MIDIPlayer::Dispose() {
  Stop();

  if (m_player) {
    DisposeMusicPlayer(m_player);
    m_player = 0;
  }

  if (m_sequence) {
    DisposeMusicSequence(m_sequence);
    m_sequence = 0;
  }

  if (m_synthNode && GlobalAUGraph::GetAUGraph()) {
    OSStatus err = AUGraphRemoveNode(GlobalAUGraph::GetAUGraph(), m_synthNode);
    ASSERTVALUE(err == noErr, err);
    m_synthNode = 0;
    m_synthUnit = 0;
  }
}

void MIDIPlayer::Reset() {
  Dispose();

  if (m_dlsURL) {
    CFRelease(m_dlsURL);
    m_dlsURL = 0;
  }

  if (m_midiURL) {
    CFRelease(m_midiURL);
    m_midiURL = 0;
  }

  m_midiName[0] = 0;
  m_dlsName[0] = 0;
  m_volume = 1.0f;
}

bool MIDIPlayer::SetMIDISource(const char *name) {
  Dispose();

  SStrCopy(m_midiName, name, sizeof(m_midiName));

  if (m_midiURL) {
    CFRelease(m_midiURL);
  }

  m_midiURL = CreateTemporaryFromMPQ(name);
  if (!m_midiURL) {
    return false;
  }

  InitializeIfReady();
  return true;
}

bool MIDIPlayer::SetDLSSource(const char *name) {
  Dispose();

  SStrCopy(m_dlsName, name, sizeof(m_dlsName));

  if (m_dlsURL) {
    CFRelease(m_dlsURL);
  }

  m_dlsURL = CreateTemporaryFromMPQ(name);
  if (!m_dlsURL) {
    return false;
  }

  InitializeIfReady();
  return true;
}

void MIDIPlayer::InitializeIfReady() {
  OSStatus                  err;
  AudioComponentDescription desc;
  UInt32                    value;

  if (!m_dlsURL || !m_midiURL) {
    return;
  }

  static struct AutoCreateWorthlessMusicDevice {
    AUNode    node;
    AudioUnit unit;

    AutoCreateWorthlessMusicDevice() {
      OSStatus                  err;
      AudioComponentDescription desc;

      desc.componentType = kAudioUnitType_MusicDevice;
      desc.componentSubType = kAudioUnitSubType_DLSSynth;
      desc.componentManufacturer = kAudioUnitManufacturer_Apple;
      desc.componentFlags = 0;
      desc.componentFlagsMask = 0;
      err = AUGraphAddNode(GlobalAUGraph::GetAUGraph(), &desc, &node);
      ASSERTVALUE(err == noErr, err);

      err = AUGraphNodeInfo(GlobalAUGraph::GetAUGraph(), node, 0, &unit);
      ASSERTVALUE(err == noErr, err);

      err = AudioUnitInitialize(unit);
      ASSERTVALUE(err == noErr, err);
    }
  } s_worthlessMusicDevice;

  desc.componentType = kAudioUnitType_MusicDevice;
  desc.componentSubType = kAudioUnitSubType_DLSSynth;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;
  err = AUGraphAddNode(GlobalAUGraph::GetAUGraph(), &desc, &m_synthNode);
  ASSERTVALUE(err == noErr, err);

  err = AUGraphNodeInfo(GlobalAUGraph::GetAUGraph(), m_synthNode, 0, &m_synthUnit);
  ASSERTVALUE(err == noErr, err);

  value = kRenderQuality_Low;
  err = AudioUnitSetProperty(m_synthUnit, kAudioUnitProperty_RenderQuality, kAudioUnitScope_Global, 0, &value, sizeof(value));
  ASSERTVALUE(err == noErr, err);

  value = 0;
  err = AudioUnitSetProperty(m_synthUnit, kMusicDeviceProperty_UsesInternalReverb, kAudioUnitScope_Global, 0, &value, sizeof(value));
  ASSERTVALUE(err == noErr, err);

  err = AudioUnitSetProperty(m_synthUnit, kMusicDeviceProperty_SoundBankURL, kAudioUnitScope_Global, 0, &m_dlsURL, sizeof(m_dlsURL));
  ASSERTVALUE(err == noErr, err);

  err = AudioUnitInitialize(m_synthUnit);
  ASSERTVALUE(err == noErr, err);

  err = NewMusicSequence(&m_sequence);
  ASSERTVALUE(err == noErr, err);

  err = MusicSequenceFileLoad(m_sequence, m_midiURL, kMusicSequenceFile_MIDIType, 0);
  ASSERTVALUE(err == noErr, err);

  err = MusicSequenceSetDestNode(m_sequence, m_synthNode, GlobalAUGraph::GetAUGraph());
  ASSERTVALUE(err == noErr, err);

  err = NewMusicPlayer(&m_player);
  ASSERTVALUE(err == noErr, err);

  err = MusicPlayerSetSequence(m_player, m_sequence);
  ASSERTVALUE(err == noErr, err);

  err = MusicPlayerSetTime(m_player, 0.0);
  ASSERTVALUE(err == noErr, err);
}

void MIDIPlayer::Start() {
  OSStatus err;

  if (!m_player || !GlobalAUGraph::GetAUGraph()) {
    return;
  }

  Mixer &mixer = GlobalAUGraph::Get().m_mixer;

  err = mixer.Connect(m_synthNode, 1);
  if (err == kAUGraphErr_InvalidConnection) {
    Console("MIDIPlayer::Start: Global mixer has no unused inputs");
    mixer.Dump(Mixer::e_dumpVerbose);
    return;
  }
  ASSERTVALUE(err == noErr, err);

  m_connected = true;
  mixer.LabelNode(m_synthNode, GetSource(), "MIDI");
  mixer.Dump(Mixer::e_dumpQuiet);

  SetVolume(m_volume);

  err = AUGraphUpdateStable(GlobalAUGraph::GetAUGraph());
  ASSERTVALUE(err == noErr, err);

  err = MusicPlayerStart(m_player);
  ASSERTVALUE(err == noErr, err);

  m_startTicks = OsGetAsyncTimeMs() + 33;
}

void MIDIPlayer::Stop() {
  OSStatus err;
  Boolean  playing;

  if (!m_player || !GlobalAUGraph::GetAUGraph()) {
    return;
  }

  while ((LONG)(OsGetAsyncTimeMs() - m_startTicks) < 0) {
    usleep(16666);
  }

  if (!m_connected) {
    return;
  }

  err = MusicPlayerIsPlaying(m_player, &playing);
  ASSERTVALUE(err == noErr, err);

  if (playing) {
    err = MusicPlayerStop(m_player);
    ASSERTVALUE(err == noErr, err);
  }

  err = GlobalAUGraph::Get().m_mixer.Disconnect(m_synthNode);
  ASSERTVALUE(err == noErr, err);

  err = AUGraphUpdateStable(GlobalAUGraph::GetAUGraph());
  ASSERTVALUE(err == noErr, err);

  m_connected = false;
}

void MIDIPlayer::SetVolume(float volume) {
  m_volume = volume;

  if (m_connected && m_player && GlobalAUGraph::GetAUGraph()) {
    OSStatus err = GlobalAUGraph::Get().m_mixer.SetMixVolume(m_synthNode, m_volume);
    ASSERTVALUE(err == noErr, err);
  }
}

bool MIDIPlayer::IsPlaying() {
  Boolean playing = false;

  if (m_player && m_connected && GlobalAUGraph::GetAUGraph()) {
    OSStatus err = MusicPlayerIsPlaying(m_player, &playing);
    ASSERTVALUE(err == noErr, err);
  }

  return playing;
}
