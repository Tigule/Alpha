#include <Base/Base.h>

#include "OsSound.h"
#include "OsSoundSynth.h"
#include "Services/AsyncFileRead.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef signed char(WINAPI *FSOUND_STREAMCALLBACK)(FSOUND_STREAM *, LPVOID, int, INT_PTR);

extern "C" FSOUND_STREAM *WINAPI FSOUND_Stream_Create(FSOUND_STREAMCALLBACK callback, int length, UINT mode, int sampleRate, INT_PTR userdata);
extern "C" signed char WINAPI    FSOUND_Stream_Close(FSOUND_STREAM *stream);
extern "C" int WINAPI            FSOUND_Stream_PlayEx(int channel, FSOUND_STREAM *stream, LPVOID dsp, signed char startPaused);
extern "C" signed char WINAPI    FSOUND_Stream_Stop(FSOUND_STREAM *stream);
extern "C" signed char WINAPI    FSOUND_SetPan(int channel, int pan);
extern "C" signed char WINAPI    FSOUND_SetPaused(int channel, signed char paused);

#define MIDI_STREAM_MODE   (0x10 | 0x40 | 0x100 | 0x2000)
#define MIDI_STREAM_FRAMES 2205
#define MIDI_BLOCK_FRAMES  220
#define MIDI_STEREOPAN     -1

enum {
  MIDI_PRIORITY_TEMPO,
  MIDI_PRIORITY_BANK_LSB,
  MIDI_PRIORITY_BANK_MSB,
  MIDI_PRIORITY_PROGRAM,
  MIDI_PRIORITY_NOTE_ON,
  MIDI_PRIORITY_NOTE_OFF,
  MIDI_PRIORITY_OTHER
};

enum {
  MIDI_PASS_ALWAYS,
  MIDI_PASS_FIRST,
  MIDI_PASS_REPEAT
};

struct MidiEvent {
  LONGLONG time;
  int      priority;
  int      pass;
  DWORD    order;
  BYTE     status;
  BYTE     data1;
  BYTE     data2;
  double   tempo;
};

struct MidiTrackEvent {
  LONGLONG time;
  LONGLONG duration;
  DWORD    order;
  BYTE     status;
  BYTE     data1;
  BYTE     data2;
  bool     removed;
};

struct MidiTimeSignature {
  LONGLONG time;
  int      beats;
  int      beat;
};

struct MidiSong {
  TSGrowableArray<MidiEvent> events;
  TSGrowableArray<DWORD>     patches;
  LONGLONG                   length;
};

struct ASYNCLOADER {
  ASYNCLOADER() : asyncLoader(NULL) {
  }
  ~ASYNCLOADER();
  void Clear();
  CAsyncObject         *asyncLoader;
  TSGrowableArray<char> buffer;
};

static const BYTE s_dbToMidi[98] = {
    127, 119, 113, 106, 100, 95, 89, 84, 80, 75, 71, 67, 63, 60, 56, 53, 50, 47, 45, 42, 40, 37, 35, 33, 31, 30, 28, 26, 25, 23, 22, 21, 20,
    19,  17,  16,  15,  15,  14, 13, 12, 11, 11, 10, 10, 9,  8,  8,  8,  7,  7,  6,  6,  6,  5,  5,  5,  4,  4,  4,  4,  3,  3,  3,  3,  3,
    2,   2,   2,   2,   2,   2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static const short s_midiToDb[128] = {
    -9600, -8415, -7211, -6506, -6006, -5619, -5302, -5034, -4802, -4598, -4415, -4249, -4098, -3959, -3830, -3710,
    -3598, -3493, -3394, -3300, -3211, -3126, -3045, -2968, -2894, -2823, -2755, -2689, -2626, -2565, -2506, -2449,
    -2394, -2341, -2289, -2238, -2190, -2142, -2096, -2050, -2006, -1964, -1922, -1881, -1841, -1802, -1764, -1726,
    -1690, -1654, -1619, -1584, -1551, -1518, -1485, -1453, -1422, -1391, -1361, -1331, -1302, -1273, -1245, -1217,
    -1190, -1163, -1137, -1110, -1085, -1059, -1034, -1010, -985,  -961,  -938,  -914,  -891,  -869,  -846,  -824,
    -802,  -781,  -759,  -738,  -718,  -697,  -677,  -657,  -637,  -617,  -598,  -579,  -560,  -541,  -522,  -504,
    -486,  -468,  -450,  -432,  -415,  -397,  -380,  -363,  -347,  -330,  -313,  -297,  -281,  -265,  -249,  -233,
    -218,  -202,  -187,  -172,  -157,  -142,  -127,  -113,  -98,   -84,   -69,   -55,   -41,   -27,   -13,   0
};

static const double s_priorityOffset[MIDI_PRIORITY_OTHER + 1] = {0.0, 1.2, 1.1, 1.0, 0.2, 0.1, 0.0};

static SCritSect      s_lock;
static CSynth        *s_synth;
static FSOUND_STREAM *s_stream;
static int            s_channel = -1;
static BYTE           s_initialized;
static BYTE           s_loading;
static ASYNCLOADER    s_MID;
static ASYNCLOADER    s_DLS;
static MidiSong      *s_song;
static CSynthBank    *s_bank;
static DWORD          s_index;
static DWORD          s_pass;
static double         s_anchorMusic;
static double         s_anchorSample;
static double         s_tempo;
static BYTE           s_pending[SYNTH_CHANNELS][128];
static int            s_volumeBase[SYNTH_CHANNELS];
static int            s_pathVolume;
static short          s_left[MIDI_BLOCK_FRAMES];
static short          s_right[MIDI_BLOCK_FRAMES];
static int            s_blockUsed = MIDI_BLOCK_FRAMES;

static int VolumeToMidi(int db) {
  if (db < -9600) {
    db = -9600;
  }
  if (db > 0) {
    db = 0;
  }
  int i = -db / 100;
  int f = -db % 100;
  return (BYTE)(s_dbToMidi[i] + f * (s_dbToMidi[i + 1] - s_dbToMidi[i]) / 100);
}

static int __cdecl CompareEvents(const void *a, const void *b) {
  const MidiEvent *x = (const MidiEvent *)a;
  const MidiEvent *y = (const MidiEvent *)b;
  if (x->time != y->time) {
    return x->time < y->time ? -1 : 1;
  }
  if (x->priority != y->priority) {
    return x->priority < y->priority ? -1 : 1;
  }
  return x->order < y->order ? -1 : x->order > y->order;
}

static int __cdecl CompareTrackEvents(const void *a, const void *b) {
  const MidiTrackEvent *x = (const MidiTrackEvent *)a;
  const MidiTrackEvent *y = (const MidiTrackEvent *)b;
  if (x->time != y->time) {
    return x->time < y->time ? -1 : 1;
  }
  return x->order < y->order ? -1 : x->order > y->order;
}

static DWORD ReadBig(const BYTE *p, int count) {
  DWORD value = 0;
  for (int i = 0; i < count; ++i) {
    value = value << 8 | p[i];
  }
  return value;
}

static bool ReadVarLength(const BYTE *&p, const BYTE *end, DWORD &value) {
  value = 0;
  for (int i = 0; i < 4; ++i) {
    if (p >= end) {
      return false;
    }
    BYTE b = *p++;
    value = value << 7 | (b & 0x7F);
    if (!(b & 0x80)) {
      return true;
    }
  }
  return false;
}

static void AddEvent(MidiSong &song, LONGLONG time, int priority, int pass, BYTE status, BYTE data1, BYTE data2, double tempo) {
  MidiEvent event;
  event.time = time;
  event.priority = priority;
  event.pass = pass;
  event.order = song.events.Count();
  event.status = status;
  event.data1 = data1;
  event.data2 = data2;
  event.tempo = tempo;
  song.events.Add(&event);
}

static bool ParseMidi(const BYTE *data, DWORD size, MidiSong &song) {
  const BYTE *p = data;
  const BYTE *end = data + size;

  if (size >= 12 && !memcmp(p, "RIFF", 4)) {
    p += 12;
  }
  if (end - p < 14 || memcmp(p, "MThd", 4)) {
    return false;
  }
  DWORD headerLength = ReadBig(p + 4, 4);
  short division = (short)ReadBig(p + 12, 2);
  if (division <= 0 || headerLength < 6 || headerLength > (DWORD)(end - p - 8)) {
    return false;
  }
  p += 8 + headerLength;

  BYTE                               bankSelect[SYNTH_CHANNELS][2];
  LONGLONG                           lastControllerTime[SYNTH_CHANNELS];
  int                                collisionOffset[SYNTH_CHANNELS];
  TSGrowableArray<MidiTimeSignature> signatures;
  TSGrowableArray<MidiTrackEvent>    track;
  TSGrowableArray<MidiTrackEvent>    notes;
  int                                timeSignatureState = 1;
  LONGLONG                           maxTick = 0;
  DWORD                              position = 0;
  DWORD                              i;

  memset(bankSelect, 0xFF, sizeof(bankSelect));
  for (i = 0; i < SYNTH_CHANNELS; ++i) {
    lastControllerTime[i] = -1;
    collisionOffset[i] = 0;
  }

  while (end - p >= 8 && !memcmp(p, "MTrk", 4)) {
    DWORD       trackLength = ReadBig(p + 4, 4);
    const BYTE *q = p + 8;
    const BYTE *trackEnd = trackLength > (DWORD)(end - q) ? end : q + trackLength;
    ULONGLONG   tick = 0;
    BYTE        runningStatus = 0;

    track.SetCount(0);

    while (q < trackEnd) {
      DWORD delta;
      if (!ReadVarLength(q, trackEnd, delta) || q >= trackEnd) {
        break;
      }
      tick += delta;
      LONGLONG time = (LONGLONG)(tick * 768 / (ULONGLONG)division);
      BYTE     status = *q;

      if (status == 0xFF) {
        if (trackEnd - q < 2) {
          break;
        }
        BYTE type = q[1];
        q += 2;
        DWORD length;
        if (!ReadVarLength(q, trackEnd, length) || length > (DWORD)(trackEnd - q)) {
          break;
        }
        if (type == 0x51 && length >= 3) {
          DWORD usPerQuarter = ReadBig(q, 3);
          if (!usPerQuarter) {
            usPerQuarter = 1;
          }
          AddEvent(song, time, MIDI_PRIORITY_TEMPO, MIDI_PASS_ALWAYS, 0xFF, 0, 0, 60000000.0 / usPerQuarter);
        } else if (type == 0x58 && length >= 2 && timeSignatureState) {
          MidiTimeSignature signature;
          signature.time = time;
          signature.beats = q[0];
          signature.beat = 1 << q[1];
          signatures.Add(&signature);
          timeSignatureState = 2;
        }
        q += length;
        continue;
      }
      if (status == 0xF0 || status == 0xF7) {
        ++q;
        DWORD length;
        if (!ReadVarLength(q, trackEnd, length) || length > (DWORD)(trackEnd - q)) {
          break;
        }
        q += length;
        continue;
      }

      if (status & 0x80) {
        runningStatus = status;
        ++q;
      } else {
        status = runningStatus;
      }
      if (!(status & 0x80)) {
        break;
      }

      int  type = status & 0xF0;
      int  channel = status & 0x0F;
      bool twoBytes = type != 0xC0 && type != 0xD0;
      if (trackEnd - q < (twoBytes ? 2 : 1)) {
        break;
      }
      MidiTrackEvent event;
      event.time = time;
      event.duration = 0;
      event.order = position++;
      event.status = (BYTE)type | (BYTE)channel;
      event.data1 = q[0] & 0x7F;
      event.data2 = twoBytes ? q[1] & 0x7F : 0;
      event.removed = false;
      q += twoBytes ? 2 : 1;

      if (type == 0x90 && !event.data2) {
        event.status = 0x80 | (BYTE)channel;
      }
      if (type == 0xB0) {
        if (time == lastControllerTime[channel]) {
          event.time += ++collisionOffset[channel];
        } else {
          collisionOffset[channel] = 0;
          lastControllerTime[channel] = time;
        }
        if (event.data1 == 0 || event.data1 == 32) {
          bankSelect[channel][event.data1 == 0 ? 1 : 0] = event.data2;
          continue;
        }
      }
      if (type == 0xC0) {
        BYTE lsb = 0;
        BYTE msb = 0;
        if (bankSelect[channel][0] != 0xFF && bankSelect[channel][1] != 0xFF) {
          lsb = bankSelect[channel][0] & 0x7F;
          msb = bankSelect[channel][1] & 0x7F;
        }
        AddEvent(song, time - 1, MIDI_PRIORITY_BANK_LSB, time ? MIDI_PASS_ALWAYS : MIDI_PASS_FIRST, 0xB0 | (BYTE)channel, 32, lsb, 0.0);
        AddEvent(song, time - 1, MIDI_PRIORITY_BANK_MSB, time ? MIDI_PASS_ALWAYS : MIDI_PASS_FIRST, 0xB0 | (BYTE)channel, 0, msb, 0.0);
        AddEvent(song, time - 1, MIDI_PRIORITY_PROGRAM, time ? MIDI_PASS_ALWAYS : MIDI_PASS_FIRST, event.status, event.data1, 0, 0.0);
        DWORD patch = event.data1 | lsb << 8 | msb << 16 | (channel == 9 ? SYNTH_DRUM_FLAG : 0);
        song.patches.Add(&patch);
        continue;
      }
      track.Add(&event);
    }

    if ((LONGLONG)(tick * 768 / (ULONGLONG)division) > maxTick) {
      maxTick = (LONGLONG)(tick * 768 / (ULONGLONG)division);
    }
    if (timeSignatureState > 1) {
      timeSignatureState = 0;
    }

    if (track.Count() > 1) {
      LONGLONG maskTime = track[track.Count() - 2].time;
      DWORD    mask = 0;
      for (int i = (int)track.Count() - 2; i >= 0; --i) {
        MidiTrackEvent &event = track[i];
        if (event.time != maskTime) {
          maskTime = event.time;
          mask = 0;
        }
        if ((event.status & 0xF0) == 0xE0) {
          if (mask & 1 << (event.status & 0x0F)) {
            event.removed = true;
          } else {
            mask |= 1 << (event.status & 0x0F);
          }
        }
      }
    }

    qsort(track.Ptr(), track.Count(), sizeof(MidiTrackEvent), CompareTrackEvents);

    TSGrowableArray<DWORD> queues[SYNTH_CHANNELS][128];
    for (i = 0; i < track.Count(); ++i) {
      MidiTrackEvent &event = track[i];
      if (event.removed) {
        continue;
      }
      int channel = event.status & 0x0F;
      if ((event.status & 0xF0) == 0x90) {
        queues[channel][event.data1].Add(&i);
      } else if ((event.status & 0xF0) == 0x80) {
        TSGrowableArray<DWORD> &queue = queues[channel][event.data1];
        if (queue.Count()) {
          MidiTrackEvent &on = track[queue[0]];
          on.duration = event.time - on.time;
          for (DWORD k = 1; k < queue.Count(); ++k) {
            queue[k - 1] = queue[k];
          }
          queue.SetCount(queue.Count() - 1);
        }
        event.removed = true;
      }
    }

    for (i = 0; i < track.Count(); ++i) {
      MidiTrackEvent &event = track[i];
      if (event.removed) {
        continue;
      }
      if ((event.status & 0xF0) == 0x90) {
        if (event.duration <= 0) {
          event.duration = 1;
        }
        notes.Add(&event);
        AddEvent(song, event.time, MIDI_PRIORITY_NOTE_ON, MIDI_PASS_ALWAYS, event.status, event.data1, event.data2, 0.0);
      } else {
        AddEvent(song, event.time, MIDI_PRIORITY_OTHER, MIDI_PASS_ALWAYS, event.status, event.data1, event.data2, 0.0);
        if (event.time > maxTick) {
          maxTick = event.time;
        }
      }
    }

    p = trackLength > (DWORD)(end - p - 8) ? end : p + 8 + trackLength;
    if (p > end) {
      break;
    }
  }

  LONGLONG endTime = maxTick;
  for (i = 0; i < notes.Count(); ++i) {
    LONGLONG start = notes[i].time;
    if (signatures.Count()) {
      LONGLONG origin = 0;
      int      beat = 4;
      for (DWORD s = 0; s < signatures.Count() && signatures[s].time <= start; ++s) {
        origin = signatures[s].time;
        beat = signatures[s].beat;
      }
      LONGLONG grid = (3072 / (beat ? beat : 4)) / 4;
      if (grid > 0) {
        start = origin + (start - origin + grid / 2) / grid * grid;
      }
    }
    if (start + notes[i].duration > endTime) {
      endTime = start + notes[i].duration;
    }
  }

  int      beats = 4;
  int      beat = 4;
  LONGLONG length = 0;
  if (signatures.Count()) {
    MidiTimeSignature &last = signatures[signatures.Count() - 1];
    beats = last.beats ? last.beats : 4;
    beat = last.beat ? last.beat : 4;
    length = last.time;
  }
  while (length < endTime) {
    length += beats * (3072 / beat);
  }
  if (length <= 0) {
    length = beats * (3072 / beat);
  }
  song.length = length;

  for (i = 0; i < notes.Count(); ++i) {
    LONGLONG off = notes[i].duration > 1 ? notes[i].time + notes[i].duration - 1 : notes[i].time + 1;
    BYTE     status = 0x80 | (notes[i].status & 0x0F);
    if (off >= length) {
      AddEvent(song, off - length, MIDI_PRIORITY_NOTE_OFF, MIDI_PASS_REPEAT, status, notes[i].data1, 0, 0.0);
    } else {
      AddEvent(song, off, MIDI_PRIORITY_NOTE_OFF, MIDI_PASS_ALWAYS, status, notes[i].data1, 0, 0.0);
    }
  }

  qsort(song.events.Ptr(), song.events.Count(), sizeof(MidiEvent), CompareEvents);
  return true;
}

static void SendSynthMessage(LONGLONG time, BYTE status, BYTE data1, BYTE data2) {
  int channel = status & 0x0F;

  if ((status & 0xF0) == 0xB0) {
    if (data1 == 7) {
      s_volumeBase[channel] = s_midiToDb[data2];
      if (s_pathVolume) {
        data2 = (BYTE)VolumeToMidi(s_pathVolume + s_volumeBase[channel]);
      }
    } else if (data1 == 121) {
      s_volumeBase[channel] = -415;
    }
  } else if ((status & 0xF0) == 0x90) {
    ++s_pending[channel][data1];
  } else if ((status & 0xF0) == 0x80) {
    if (!s_pending[channel][data1]) {
      return;
    }
    --s_pending[channel][data1];
  }

  s_synth->PlayMessage(time, status, data1, data2);
}

static void StopSong() {
  if (!s_song) {
    return;
  }

  LONGLONG time = s_synth->Time();
  for (int channel = 0; channel < SYNTH_CHANNELS; ++channel) {
    for (int key = 0; key < 128; ++key) {
      while (s_pending[channel][key]) {
        SendSynthMessage(time, 0x80 | (BYTE)channel, (BYTE)key, 0);
      }
    }
  }

  s_synth->UnloadAll();
  delete s_song;
  s_song = NULL;
  if (s_bank) {
    s_bank->Release();
    s_bank = NULL;
  }
}

static void Schedule(LONGLONG blockEnd) {
  if (!s_song) {
    return;
  }

  for (;;) {
    double samplesPerTick = 60.0 * SYNTH_SAMPLE_RATE / (s_tempo * 768.0);

    if (s_index >= s_song->events.Count()) {
      double loop = s_anchorSample + (s_song->length - s_anchorMusic) * samplesPerTick;
      if (loop >= blockEnd) {
        return;
      }
      s_anchorSample = loop;
      s_anchorMusic = 0.0;
      s_index = 0;
      ++s_pass;
      continue;
    }

    const MidiEvent &event = s_song->events[s_index];
    if ((event.pass == MIDI_PASS_FIRST && s_pass) || (event.pass == MIDI_PASS_REPEAT && !s_pass)) {
      ++s_index;
      continue;
    }

    double at = s_anchorSample + (event.time - s_anchorMusic) * samplesPerTick;
    if (at >= blockEnd) {
      return;
    }
    ++s_index;

    if (event.priority == MIDI_PRIORITY_TEMPO) {
      s_anchorSample = at;
      s_anchorMusic = (double)event.time;
      s_tempo = event.tempo;
      continue;
    }

    at -= s_priorityOffset[event.priority] * SYNTH_SAMPLE_RATE / 1000000.0;
    SendSynthMessage((LONGLONG)floor(at), event.status, event.data1, event.data2);
  }
}

static signed char WINAPI StreamCallback(FSOUND_STREAM *, LPVOID buffer, int length, INT_PTR) {
  short *out = (short *)buffer;
  int    frames = length / 4;

  s_lock.Enter();
  while (frames > 0) {
    if (s_blockUsed == MIDI_BLOCK_FRAMES) {
      Schedule(s_synth->Time() + MIDI_BLOCK_FRAMES);
      s_synth->Mix(s_left, s_right, MIDI_BLOCK_FRAMES);
      s_blockUsed = 0;
    }
    int count = MIDI_BLOCK_FRAMES - s_blockUsed;
    if (count > frames) {
      count = frames;
    }
    for (int i = 0; i < count; ++i) {
      *out++ = s_left[s_blockUsed + i];
      *out++ = s_right[s_blockUsed + i];
    }
    s_blockUsed += count;
    frames -= count;
  }
  s_lock.Leave();

  return 1;
}

static void PostLoadCallback(LPVOID userArg) {
  if (!s_loading || !s_MID.asyncLoader || !s_MID.asyncLoader->isLoaded || !s_DLS.asyncLoader || !s_DLS.asyncLoader->isLoaded) {
    return;
  }
  s_loading = 0;

  CSynthBank *bank = new CSynthBank;
  MidiSong   *song = new MidiSong;
  if (!bank->Load((const BYTE *)s_DLS.buffer.Ptr(), s_DLS.buffer.Count()) ||
      !ParseMidi((const BYTE *)s_MID.buffer.Ptr(), s_MID.buffer.Count(), *song))
  {
    bank->Release();
    delete song;
    return;
  }

  s_lock.Enter();
  StopSong();
  s_song = song;
  s_bank = bank;
  for (DWORD i = 0; i < song->patches.Count(); ++i) {
    s_synth->Download(bank, song->patches[i]);
  }
  s_index = 0;
  s_pass = 0;
  s_tempo = 120.0;
  s_anchorMusic = 0.0;
  s_anchorSample = (double)s_synth->Time();
  s_lock.Leave();
}

static void InitLoader(ASYNCLOADER &loader, LPCSTR fileName) {
  loader.Clear();

  SFile *file = NULL;
  if (!SFile::Open(fileName, &file)) {
    return;
  }

  CAsyncObject *asyncLoader = AsyncFileReadCreateObject();
  if (!asyncLoader) {
    SFile::Close(file);
    return;
  }

  loader.buffer.SetCount(SFile::GetFileSize(file, 0));
  asyncLoader->isLoaded = 0;
  asyncLoader->userPostloadCallback = PostLoadCallback;
  asyncLoader->file = file;
  asyncLoader->offset = 0;
  asyncLoader->size = SFile::GetFileSize(file, 0);
  asyncLoader->buffer = loader.buffer.Ptr();
  asyncLoader->userArg = NULL;
  loader.asyncLoader = asyncLoader;
  AsyncFileReadObject(asyncLoader);
}

BOOL Sound::MIDI_Initialize() {
  if (s_initialized) {
    return TRUE;
  }

  s_synth = new CSynth;
  for (int i = 0; i < SYNTH_CHANNELS; ++i) {
    s_volumeBase[i] = -415;
  }
  s_pathVolume = 0;
  s_blockUsed = MIDI_BLOCK_FRAMES;

  s_stream = FSOUND_Stream_Create(StreamCallback, MIDI_STREAM_FRAMES * 4, MIDI_STREAM_MODE, SYNTH_SAMPLE_RATE, 0);
  if (s_stream) {
    s_channel = FSOUND_Stream_PlayEx(-1, s_stream, 0, 1);
    if (s_channel != -1) {
      FSOUND_SetPan(s_channel, MIDI_STEREOPAN);
      FSOUND_SetPaused(s_channel, 0);
    }
  }
  if (s_channel == -1) {
    if (s_stream) {
      FSOUND_Stream_Close(s_stream);
      s_stream = NULL;
    }
    delete s_synth;
    s_synth = NULL;
    return FALSE;
  }

  s_initialized = 1;
  return TRUE;
}

void Sound::MIDI_Shutdown() {
  MIDI_Stop();
  if (s_stream) {
    FSOUND_Stream_Stop(s_stream);
    FSOUND_Stream_Close(s_stream);
    s_stream = NULL;
    s_channel = -1;
  }
  if (s_synth) {
    delete s_synth;
    s_synth = NULL;
  }
  s_initialized = 0;
  s_loading = 0;
  s_MID.Clear();
  s_DLS.Clear();
}

void Sound::MIDI_Play(LPCSTR midiFilename, LPCSTR dlsFilename) {
  if (s_initialized && midiFilename && *midiFilename && dlsFilename && *dlsFilename) {
    s_loading = 1;
    InitLoader(s_MID, midiFilename);
    InitLoader(s_DLS, dlsFilename);
    s_lock.Enter();
    StopSong();
    s_lock.Leave();
  }
}

void Sound::MIDI_Stop() {
  if (s_initialized) {
    s_lock.Enter();
    StopSong();
    s_lock.Leave();
  }
}

void Sound::MIDI_SetVolume(float volume) {
  if (!s_initialized) {
    return;
  }

  ASSERT(volume >= 0.0f && volume <= 1.0f);
  long level = -9600 - (long)(volume * -9600.0f);
  if (level < -9600) {
    level = -9600;
  }
  if (level > 0) {
    level = 0;
  }
  int i = -level / 100;
  int f = -level % 100;
  int m = s_dbToMidi[i] + f * (s_dbToMidi[i + 1] - s_dbToMidi[i]) / 100;

  s_lock.Enter();
  s_pathVolume = s_midiToDb[m];
  LONGLONG time = s_synth->Time();
  for (int channel = 0; channel < SYNTH_CHANNELS; ++channel) {
    s_synth->PlayMessage(time, 0xB0 | (BYTE)channel, 7, (BYTE)VolumeToMidi(s_pathVolume + s_volumeBase[channel]));
  }
  s_lock.Leave();
}

bool Sound::MIDI_Playing() {
  return s_song != NULL;
}

void ASYNCLOADER::Clear() {
  if (asyncLoader) {
    SFile *file = asyncLoader->file;
    AsyncFileReadDestroyObject(asyncLoader);
    asyncLoader = NULL;
    if (file) {
      SFile::Close(file);
    }
  }
}

ASYNCLOADER::~ASYNCLOADER() {
  buffer.Clear();
  Clear();
}
