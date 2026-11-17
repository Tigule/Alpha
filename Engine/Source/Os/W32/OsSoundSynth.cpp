#include <Base/Base.h>

#include "OsSoundSynth.h"

#include <math.h>
#include <string.h>

#define SYNTH_MAX_SPAN ((SYNTH_SAMPLE_RATE + 19) / 20)
#define SYNTH_FOREVER  ((LONGLONG)0x7FFFFFFF << 32 | 0xFFFFFFFF)

static int   s_dbToVolume[1001];
static int   s_cents[201];
static int   s_semitones[97];
static short s_sine[256];
static short s_attack[201];
static int   s_panToLevel[128];
static int   s_midiToLevel[128];
static bool  s_tablesReady;

static const DWORD s_channelPriority[SYNTH_CHANNELS] = {
    0x8000000E, 0x8000000D, 0x8000000C, 0x8000000B, 0x8000000A, 0x80000009, 0x80000008, 0x80000007,
    0x80000006, 0x8000000F, 0x80000005, 0x80000004, 0x80000003, 0x80000002, 0x80000001, 0x80000000
};

static void InitTables() {
  int i;
  if (s_tablesReady) {
    return;
  }

  for (i = -1000; i <= 0; ++i) {
    s_dbToVolume[i + 1000] = (int)(pow(pow(10.0, i / 100.0), 0.5) * 4095.0);
  }
  for (int c = -100; c <= 100; ++c) {
    s_cents[c + 100] = (int)(pow(2.0, c / 1200.0) * 4096.0);
  }
  for (int s = -48; s <= 48; ++s) {
    s_semitones[s + 48] = (int)(pow(2.0, s / 12.0) * 4096.0);
  }
  for (i = 0; i < 256; ++i) {
    s_sine[i] = (short)(int)(sin(6.283185307 * i * 0.00390625) * 100.0);
  }
  s_attack[0] = 0;
  for (i = 1; i <= 200; ++i) {
    s_attack[i] = (short)(int)(log10((i / 200.0) * (i / 200.0)) * 10000.0 / 96.0 + 1000.0);
  }
  s_panToLevel[0] = -2500;
  s_midiToLevel[0] = -9600;
  for (int p = 1; p < 128; ++p) {
    s_panToLevel[p] = (int)(log10(p / 127.0) * 1000.0);
    s_midiToLevel[p] = (int)(4000.0 * log10(p / 127.0));
  }

  s_tablesReady = true;
}

static int LevelToVolume(int level) {
  int index = level / 10;
  if (index < -1000) {
    index = -1000;
  }
  if (index > 0) {
    index = 0;
  }
  return s_dbToVolume[index + 1000];
}

static int CentsToPitch(int cents) {
  if (cents >= -100 && cents <= 100) {
    return s_cents[cents + 100];
  }
  if (cents > 100) {
    if (cents > 4800) {
      cents = 4800;
    }
    return (s_semitones[(cents / 100) % 12 + 48] * (s_cents[cents % 100 + 100] << (cents / 1200))) >> 12;
  }
  if (cents < -4800) {
    cents = -4800;
  }
  int n = -cents;
  return (s_semitones[48 - (n / 100) % 12] * (s_cents[100 - n % 100] >> (n / 1200))) >> 12;
}

static LONGLONG TimeCentsToSamples(LONG timeCents, int maxSeconds) {
  if (timeCents == (LONG)0x80000000) {
    return 0;
  }
  LONGLONG samples = (LONGLONG)(pow(2.0, timeCents / 78643200.0) * SYNTH_SAMPLE_RATE);
  if (samples > (LONGLONG)maxSeconds * SYNTH_SAMPLE_RATE) {
    samples = (LONGLONG)maxSeconds * SYNTH_SAMPLE_RATE;
  }
  return samples;
}

static LONG MulDivRound(LONG a, LONG b, LONG c) {
  LONGLONG product = (LONGLONG)a * b;
  LONGLONG half = c / 2;
  if (product >= 0) {
    return (LONG)((product + half) / c);
  }
  return (LONG)-((-product + half) / c);
}

static short Saturate(int value) {
  if (value > 32767) {
    return 32767;
  }
  if (value < -32768) {
    return -32768;
  }
  return (short)value;
}

static DWORD ReadDword(const BYTE *p) {
  return p[0] | p[1] << 8 | p[2] << 16 | (DWORD)p[3] << 24;
}

static WORD ReadWord(const BYTE *p) {
  return (WORD)(p[0] | p[1] << 8);
}

static bool IsChunk(const BYTE *p, const char *id) {
  return p[0] == id[0] && p[1] == id[1] && p[2] == id[2] && p[3] == id[3];
}

static void InitArticulation(SynthArticulation &art) {
  memset(&art, 0, sizeof(art));
  art.sustain = 1000;
  art.lfoFrequency = 0x5000000 / SYNTH_SAMPLE_RATE;
}

static short ClampShort(LONG value, LONG limit) {
  if (value > limit) {
    return (short)limit;
  }
  if (value < -limit) {
    return (short)-limit;
  }
  return (short)value;
}

static void ReadArticulation(const BYTE *data, DWORD size, SynthArticulation &art) {
  if (size < 8) {
    return;
  }

  DWORD header = ReadDword(data);
  DWORD count = ReadDword(data + 4);
  const BYTE *connection = data + header;

  for (DWORD i = 0; i < count && connection + 12 <= data + size; ++i, connection += 12) {
    WORD source = ReadWord(connection);
    WORD control = ReadWord(connection + 2);
    WORD destination = ReadWord(connection + 4);
    LONG scale = (LONG)ReadDword(connection + 8);
    short gain = (short)((DWORD)(10 * scale) >> 16);
    short high = (short)((DWORD)scale >> 16);

    if (source == 0 && control == 0) {
      switch (destination) {
        case 0x0004:
          art.pan = (short)((scale >> 12) / 125);
          break;
        case 0x0104:
          art.lfoFrequency = (DWORD)(LONGLONG)(pow(2.0, (scale / 65536.0 - 6900.0) / 1200.0) * 7381975040.0 / SYNTH_SAMPLE_RATE);
          break;
        case 0x0105:
          art.lfoDelay = TimeCentsToSamples(scale, 10);
          break;
        case 0x0206:
          art.attack = TimeCentsToSamples(scale, 40);
          break;
        case 0x0207:
          art.decay = TimeCentsToSamples(scale, 40);
          break;
        case 0x0209:
          art.release = TimeCentsToSamples(scale, 40);
          break;
        case 0x020A:
          art.sustain = high;
          break;
        case 0x020B:
          art.delay = TimeCentsToSamples(scale, 40);
          break;
        case 0x020C:
          art.hold = TimeCentsToSamples(scale, 40);
          break;
      }
    } else if (source == 0x0001) {
      if (control == 0) {
        if (destination == 0x0001) {
          art.lfoGainScale = gain;
        } else if (destination == 0x0003) {
          art.lfoPitchScale = high;
        }
      } else if (control == 0x0081) {
        if (destination == 0x0001) {
          art.lfoModWheelToGain = gain;
        } else if (destination == 0x0003) {
          art.lfoModWheelToPitch = high;
        }
      } else if (control == 0x0008) {
        if (destination == 0x0001) {
          art.lfoPressureToGain = gain;
        } else if (destination == 0x0003) {
          art.lfoPressureToPitch = high;
        }
      }
    } else if (source == 0x0002 && destination == 0x0206) {
      art.velocityToAttack = high;
    } else if (source == 0x0003 && destination == 0x0207) {
      art.keyToDecay = high;
    } else if (source == 0x0003 && destination == 0x020C) {
      art.keyToHold = high;
    }
  }

  if (art.attack > 1764000) {
    art.attack = 1764000;
  }
  if (art.decay > 1764000) {
    art.decay = 1764000;
  }
  if (art.release > 1764000) {
    art.release = 1764000;
  }
  if (art.sustain < 0) {
    art.sustain = 0;
  }
  if (art.sustain > 1000) {
    art.sustain = 1000;
  }
  art.velocityToAttack = ClampShort(art.velocityToAttack, 12000);
  art.keyToDecay = ClampShort(art.keyToDecay, 12000);
  art.keyToHold = ClampShort(art.keyToHold, 12000);
  if (art.lfoFrequency < 64) {
    art.lfoFrequency = 64;
  }
  if (art.lfoFrequency > 7600) {
    art.lfoFrequency = 7600;
  }
  if (art.lfoDelay > 441000) {
    art.lfoDelay = 441000;
  }
  art.lfoModWheelToGain = ClampShort(art.lfoModWheelToGain, 1200);
  art.lfoModWheelToPitch = ClampShort(art.lfoModWheelToPitch, 1200);
  art.lfoGainScale = ClampShort(art.lfoGainScale, 1200);
  art.lfoPitchScale = ClampShort(art.lfoPitchScale, 1200);
  art.lfoPressureToGain = ClampShort(art.lfoPressureToGain, 1200);
  art.lfoPressureToPitch = ClampShort(art.lfoPressureToPitch, 1200);
}

static bool ReadArticulationList(const BYTE *data, DWORD size, SynthArticulation &art) {
  bool found = false;
  for (DWORD at = 0; at + 8 <= size;) {
    DWORD length = ReadDword(data + at + 4);
    if (length > size - at - 8) {
      break;
    }
    if (IsChunk(data + at, "art1") || IsChunk(data + at, "art2")) {
      ReadArticulation(data + at + 8, length, art);
      found = true;
    }
    at += 8 + length + (length & 1);
  }
  return found;
}

static void ReadSample(const BYTE *data, DWORD size, WORD &unityNote, short &fineTune, LONG &attenuation, DWORD &loopCount, DWORD &loopType, DWORD &loopStart, DWORD &loopLength) {
  if (size < 20) {
    return;
  }

  DWORD header = ReadDword(data);
  unityNote = ReadWord(data + 4);
  fineTune = (short)ReadWord(data + 6);
  attenuation = (LONG)ReadDword(data + 8);
  loopCount = ReadDword(data + 16);
  loopType = 0;
  loopStart = 0;
  loopLength = 0;
  if (loopCount && header + 16 <= size) {
    loopType = ReadDword(data + header + 4);
    loopStart = ReadDword(data + header + 8);
    loopLength = ReadDword(data + header + 12);
  }
}

CSynthBank::CSynthBank() : m_refs(1) {
}

CSynthBank::~CSynthBank() {
  for (DWORD i = 0; i < m_waves.Count(); ++i) {
    FREEIFUSED(m_waves[i].data);
  }
}

void CSynthBank::AddRef() {
  ++m_refs;
}

void CSynthBank::Release() {
  if (--m_refs == 0) {
    delete this;
  }
}

const SynthInstrument *CSynthBank::Find(DWORD patch) const {
  for (DWORD i = 0; i < m_instruments.Count(); ++i) {
    if (m_instruments[i].patch == patch) {
      return &m_instruments[i];
    }
  }
  return 0;
}

bool CSynthBank::Load(const BYTE *data, DWORD size) {
  InitTables();

  if (size < 12 || !IsChunk(data, "RIFF") || !IsChunk(data + 8, "DLS ")) {
    return false;
  }

  TSGrowableArray<DWORD> cues;
  TSGrowableArray<DWORD> waveOffsets;
  DWORD                  end = 8 + ReadDword(data + 4);
  if (end > size) {
    end = size;
  }

  for (DWORD at = 12; at + 8 <= end;) {
    DWORD length = ReadDword(data + at + 4);
    if (length > end - at - 8) {
      break;
    }
    const BYTE *chunk = data + at + 8;

    if (IsChunk(data + at, "ptbl") && length >= 8) {
      DWORD header = ReadDword(chunk);
      DWORD count = ReadDword(chunk + 4);
      for (DWORD i = 0; i < count && header + i * 4 + 4 <= length; ++i) {
        DWORD cue = ReadDword(chunk + header + i * 4);
        cues.Add(&cue);
      }
    } else if (IsChunk(data + at, "LIST") && length >= 4 && IsChunk(chunk, "wvpl")) {
      for (DWORD w = 4; w + 12 <= length;) {
        DWORD waveLength = ReadDword(chunk + w + 4);
        if (waveLength > length - w - 8) {
          break;
        }
        if (IsChunk(chunk + w, "LIST") && IsChunk(chunk + w + 8, "wave")) {
          SynthWave wave;
          memset(&wave, 0, sizeof(wave));
          WORD        format = 0;
          WORD        channels = 0;
          WORD        bits = 0;
          const BYTE *samples = 0;
          DWORD       sampleBytes = 0;
          const BYTE *part = chunk + w + 12;
          for (DWORD p = 0; p + 8 <= waveLength - 4;) {
            DWORD partLength = ReadDword(part + p + 4);
            if (partLength > waveLength - 4 - p - 8) {
              break;
            }
            if (IsChunk(part + p, "fmt ") && partLength >= 16) {
              format = ReadWord(part + p + 8);
              channels = ReadWord(part + p + 10);
              wave.sampleRate = ReadDword(part + p + 12);
              bits = ReadWord(part + p + 22);
            } else if (IsChunk(part + p, "data")) {
              samples = part + p + 8;
              sampleBytes = partLength;
            } else if (IsChunk(part + p, "wsmp")) {
              wave.hasSample = TRUE;
              ReadSample(part + p + 8, partLength, wave.unityNote, wave.fineTune, wave.attenuation, wave.loopCount, wave.loopType, wave.loopStart, wave.loopLength);
            }
            p += 8 + partLength + (partLength & 1);
          }
          if (format == 1 && channels == 1 && bits == 16 && samples) {
            wave.numSamples = sampleBytes / 2;
            wave.data = (short *)ALLOC((wave.numSamples + 1) * sizeof(short));
            for (DWORD i = 0; i < wave.numSamples; ++i) {
              wave.data[i] = (short)ReadWord(samples + i * 2);
            }
            wave.data[wave.numSamples] = 0;
          }
          if (wave.sampleRate < 3000) {
            wave.sampleRate = 3000;
          }
          if (wave.sampleRate > 200000) {
            wave.sampleRate = 200000;
          }
          DWORD offset = w - 4;
          m_waves.Add(&wave);
          waveOffsets.Add(&offset);
        }
        w += 8 + waveLength + (waveLength & 1);
      }
    } else if (IsChunk(data + at, "LIST") && length >= 4 && IsChunk(chunk, "lins")) {
      for (DWORD i = 4; i + 12 <= length;) {
        DWORD insLength = ReadDword(chunk + i + 4);
        if (insLength > length - i - 8) {
          break;
        }
        if (IsChunk(chunk + i, "LIST") && IsChunk(chunk + i + 8, "ins ")) {
          SynthInstrument instrument;
          InitArticulation(instrument.articulation);
          instrument.patch = 0;
          instrument.firstRegion = m_regions.Count();
          instrument.numRegions = 0;
          const BYTE *part = chunk + i + 12;
          for (DWORD p = 0; p + 8 <= insLength - 4;) {
            DWORD partLength = ReadDword(part + p + 4);
            if (partLength > insLength - 4 - p - 8) {
              break;
            }
            const BYTE *body = part + p + 8;
            if (IsChunk(part + p, "insh") && partLength >= 12) {
              DWORD bank = ReadDword(body + 4);
              DWORD program = ReadDword(body + 8);
              instrument.patch = (bank & SYNTH_DRUM_FLAG) | ((bank >> 8) & 0x7F) << 16 | (bank & 0x7F) << 8 | (program & 0x7F);
            } else if (IsChunk(part + p, "LIST") && partLength >= 4 && (IsChunk(body, "lart") || IsChunk(body, "lar2"))) {
              ReadArticulationList(body + 4, partLength - 4, instrument.articulation);
            } else if (IsChunk(part + p, "LIST") && partLength >= 4 && IsChunk(body, "lrgn")) {
              for (DWORD r = 4; r + 12 <= partLength;) {
                DWORD rgnLength = ReadDword(body + r + 4);
                if (rgnLength > partLength - r - 8) {
                  break;
                }
                if (IsChunk(body + r, "LIST") && (IsChunk(body + r + 8, "rgn ") || IsChunk(body + r + 8, "rgn2"))) {
                  SynthRegion region;
                  memset(&region, 0, sizeof(region));
                  region.keyHigh = 127;
                  region.velocityHigh = 127;
                  region.wave = 0xFFFFFFFF;
                  InitArticulation(region.articulation);
                  BOOL  hasSample = FALSE;
                  WORD  unityNote = 60;
                  short fineTune = 0;
                  LONG  attenuation = 0;
                  DWORD loopCount = 0;
                  DWORD loopType = 0;
                  DWORD loopStart = 0;
                  DWORD loopLength = 0;
                  const BYTE *item = body + r + 12;
                  for (DWORD q = 0; q + 8 <= rgnLength - 4;) {
                    DWORD itemLength = ReadDword(item + q + 4);
                    if (itemLength > rgnLength - 4 - q - 8) {
                      break;
                    }
                    const BYTE *itemBody = item + q + 8;
                    if (IsChunk(item + q, "rgnh") && itemLength >= 12) {
                      region.keyLow = ReadWord(itemBody);
                      region.keyHigh = ReadWord(itemBody + 2);
                      region.velocityLow = ReadWord(itemBody + 4);
                      region.velocityHigh = ReadWord(itemBody + 6);
                      region.options = ReadWord(itemBody + 8);
                    } else if (IsChunk(item + q, "wsmp")) {
                      hasSample = TRUE;
                      ReadSample(itemBody, itemLength, unityNote, fineTune, attenuation, loopCount, loopType, loopStart, loopLength);
                    } else if (IsChunk(item + q, "wlnk") && itemLength >= 12) {
                      region.wave = ReadDword(itemBody + 8);
                    } else if (IsChunk(item + q, "LIST") && itemLength >= 4 && (IsChunk(itemBody, "lart") || IsChunk(itemBody, "lar2"))) {
                      region.hasArticulation = ReadArticulationList(itemBody + 4, itemLength - 4, region.articulation);
                    }
                    q += 8 + itemLength + (itemLength & 1);
                  }
                  region.unityNote = unityNote;
                  region.fineTune = fineTune;
                  region.attenuation = (DWORD)(10 * attenuation) >> 16;
                  region.loopType = loopType;
                  region.loopStart = loopStart;
                  region.loopEnd = loopStart + loopLength;
                  region.looped = loopCount != 0;
                  if (!hasSample) {
                    region.unityNote = 0xFFFF;
                  }
                  m_regions.Add(&region);
                  ++instrument.numRegions;
                }
                r += 8 + rgnLength + (rgnLength & 1);
              }
            }
            p += 8 + partLength + (partLength & 1);
          }
          m_instruments.Add(&instrument);
        }
        i += 8 + insLength + (insLength & 1);
      }
    }
    at += 8 + length + (length & 1);
  }

  for (DWORD i = 0; i < m_regions.Count(); ++i) {
    SynthRegion &region = m_regions[i];
    DWORD        cue = region.wave < cues.Count() ? cues[region.wave] : 0xFFFFFFFF;
    region.wave = 0xFFFFFFFF;
    for (DWORD w = 0; w < waveOffsets.Count(); ++w) {
      if (waveOffsets[w] == cue) {
        region.wave = w;
        break;
      }
    }
    if (region.wave == 0xFFFFFFFF || !m_waves[region.wave].data) {
      region.wave = 0xFFFFFFFF;
      continue;
    }

    SynthWave &wave = m_waves[region.wave];
    if (region.unityNote == 0xFFFF) {
      region.unityNote = wave.hasSample ? wave.unityNote : 60;
      region.fineTune = wave.hasSample ? wave.fineTune : 0;
      region.attenuation = wave.hasSample ? (int)((DWORD)(10 * wave.attenuation) >> 16) : 0;
      region.looped = wave.hasSample && wave.loopCount != 0;
      region.loopType = wave.loopType;
      region.loopStart = wave.loopStart;
      region.loopEnd = wave.loopStart + wave.loopLength;
    }
    region.attenuation = (short)region.attenuation;
    if (region.unityNote > 127) {
      region.unityNote = 127;
    }
    if (region.fineTune > 1200) {
      region.fineTune = 1200;
    }
    if (region.fineTune < -1200) {
      region.fineTune = -1200;
    }
    if (region.loopEnd > wave.numSamples) {
      region.loopEnd = wave.numSamples;
    }
    if (region.loopEnd == 0) {
      region.loopEnd = 1;
    }
    if (region.loopStart > region.loopEnd) {
      region.loopStart = region.loopEnd;
    }
    if (region.loopEnd - region.loopStart < 6 || region.loopEnd - region.loopStart >= 0x80000) {
      region.looped = FALSE;
    }
    if (region.looped) {
      wave.data[wave.numSamples] = wave.data[region.loopStart < wave.numSamples ? region.loopStart : 0];
    } else {
      wave.data[wave.numSamples] = 0;
    }
  }

  return true;
}

CSynth::CSynth() : m_time(0), m_serial(0), m_order(0) {
  InitTables();

  for (int i = 0; i < SYNTH_CHANNELS; ++i) {
    SynthChannel &channel = m_channels[i];
    channel.modWheel.value = 0;
    channel.volume.value = 100;
    channel.pan.value = 64;
    channel.expression.value = 127;
    channel.bend.value = 0x2000;
    channel.pressure.value = 0;
    channel.program = 0;
    channel.bankMSB = 0;
    channel.bankLSB = 0;
    channel.latchedMSB = 0;
    channel.latchedLSB = 0;
    channel.bendRange = 200;
    channel.fineTune = 0;
    channel.coarseTune = 0;
    channel.rpn = 0x3FFF;
    channel.dataMSB = 0;
    channel.dataLSB = 0;
    channel.sustain = false;
    channel.mono = false;
    channel.priority = s_channelPriority[i];
  }

  memset(m_voices, 0, sizeof(m_voices));
}

CSynth::~CSynth() {
  for (int i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
    if (m_voices[i].inUse) {
      ClearVoice(&m_voices[i]);
    }
  }
  UnloadAll();
}

LONGLONG CSynth::Time() const {
  return m_time;
}

void CSynth::Download(CSynthBank *bank, DWORD patch) {
  for (DWORD i = 0; i < m_downloads.Count(); ++i) {
    if (m_downloads[i].patch == patch && m_downloads[i].bank == bank) {
      return;
    }
  }

  const SynthInstrument *instrument = bank->Find(patch);
  if (!instrument) {
    return;
  }

  SynthDownload download;
  download.patch = patch;
  download.bank = bank;
  download.instrument = instrument;
  bank->AddRef();
  m_downloads.Add(&download);
}

void CSynth::UnloadAll() {
  for (DWORD i = 0; i < m_downloads.Count(); ++i) {
    m_downloads[i].bank->Release();
  }
  m_downloads.SetCount(0);
}

void CSynth::PlayMessage(LONGLONG time, BYTE status, BYTE data1, BYTE data2) {
  SynthEvent event;
  event.time = time;
  event.status = status;
  event.data1 = data1;
  event.data2 = data2;

  DWORD at = m_events.Count();
  while (at > 0 && m_events[at - 1].time > time) {
    --at;
  }
  m_events.Add(&event);
  for (DWORD i = m_events.Count() - 1; i > at; --i) {
    m_events[i] = m_events[i - 1];
  }
  m_events[at] = event;
}

static void Record(SynthRecorder &recorder, LONGLONG time, int value) {
  DWORD count = recorder.events.Count();
  if (count && recorder.events[count - 1].time == time && recorder.events[count - 1].value == value) {
    return;
  }
  SynthControl control;
  control.time = time;
  control.value = value;
  recorder.events.Add(&control);
}

static int GetControl(const SynthRecorder &recorder, LONGLONG time) {
  int value = recorder.value;
  for (DWORD i = 0; i < recorder.events.Count() && recorder.events[i].time <= time; ++i) {
    value = recorder.events[i].value;
  }
  return value;
}

static void FlushControl(SynthRecorder &recorder) {
  if (recorder.events.Count()) {
    recorder.value = recorder.events[recorder.events.Count() - 1].value;
    recorder.events.SetCount(0);
  }
}

static int EnvelopeLevel(const SynthVoice *voice, LONGLONG time, LONGLONG *next) {
  if (time > voice->egStop) {
    LONGLONG elapsed = time - voice->egStop;
    if (elapsed >= voice->egRelease) {
      *next = SYNTH_FOREVER;
      return 0;
    }
    LONGLONG ignored;
    LONGLONG level = EnvelopeLevel(voice, voice->egStop, &ignored);
    level = level * (voice->egRelease - elapsed) / voice->egRelease;
    if (elapsed < (voice->egRelease >> 2) - 100) {
      *next = (voice->egRelease >> 2) - elapsed;
    } else if (elapsed < (voice->egRelease >> 1) - 100) {
      *next = (voice->egRelease >> 1) - elapsed;
    } else {
      *next = voice->egRelease - elapsed;
    }
    return (int)level;
  }

  LONGLONG elapsed = time - voice->start;
  if (elapsed < voice->egDelay) {
    *next = voice->egDelay - elapsed;
    return 0;
  }
  elapsed -= voice->egDelay;
  if (elapsed < voice->egAttack) {
    LONGLONG level = voice->egAttack ? 1000 * elapsed / voice->egAttack : 0;
    if (level < 0) {
      level = 0;
    }
    if (level > 1000) {
      level = 1000;
    }
    *next = voice->egAttack - elapsed;
    return s_attack[level / 5];
  }
  elapsed -= voice->egAttack;
  if (elapsed < voice->egHold) {
    *next = voice->egHold - elapsed;
    return 1000;
  }
  elapsed -= voice->egHold;
  if (elapsed >= voice->egDecay) {
    *next = 44100;
    return voice->egSustain;
  }
  LONGLONG level = 1000 - (1000 - voice->egSustain) * elapsed / voice->egDecay;
  if (elapsed < (voice->egDecay >> 2) - 100) {
    *next = (voice->egDecay >> 2) - elapsed;
  } else if (elapsed < (voice->egDecay >> 1) - 100) {
    *next = (voice->egDecay >> 1) - elapsed;
  } else {
    *next = voice->egDecay - elapsed;
  }
  return (int)level;
}

static int LfoLevel(const SynthVoice *voice, LONGLONG time, LONGLONG *next) {
  LONGLONG begin = voice->start + voice->articulation->lfoDelay;
  if (time < begin) {
    *next = begin - time;
    return 0;
  }
  *next = voice->lfoInterval;
  return s_sine[(BYTE)(((DWORD)voice->articulation->lfoFrequency * (DWORD)(time - begin)) >> 16)];
}

void CSynth::ClearVoice(SynthVoice *voice) {
  voice->bank->Release();
  voice->inUse = false;
  voice->bank = 0;
}

void CSynth::StopVoice(SynthVoice *voice, LONGLONG time) {
  if (voice->egStop != SYNTH_FOREVER) {
    return;
  }
  LONGLONG ignored;
  voice->egRelease = voice->egRelease * EnvelopeLevel(voice, time, &ignored) / 1000;
  voice->egStop = time;
  voice->sustained = false;
  if (voice->region->loopType == 1) {
    voice->looped = FALSE;
  }
}

void CSynth::QuickStopVoice(SynthVoice *voice, LONGLONG time) {
  LONGLONG ignored;
  voice->tag = true;
  if (voice->egStop != SYNTH_FOREVER) {
    time = voice->egStop;
  } else if (time <= voice->start) {
    time = voice->start + 1;
  }
  voice->egRelease = voice->egRelease * EnvelopeLevel(voice, time, &ignored) / 1000;
  if (voice->egRelease > SYNTH_SAMPLE_RATE / 70) {
    voice->egRelease = SYNTH_SAMPLE_RATE / 70;
  }
  voice->egStop = time;
  voice->sustained = false;
}

SynthVoice *CSynth::OldestVoice() {
  SynthVoice *best = 0;
  for (int i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
    SynthVoice *voice = &m_voices[i];
    if (!voice->inUse || voice->tag) {
      continue;
    }
    if (!best) {
      best = voice;
      continue;
    }
    DWORD priority = m_channels[voice->channel].priority;
    DWORD bestPriority = m_channels[best->channel].priority;
    bool  released = voice->egStop != SYNTH_FOREVER;
    bool  bestReleased = best->egStop != SYNTH_FOREVER;
    if (priority < bestPriority) {
      best = voice;
    } else if (priority == bestPriority) {
      if (released && (!bestReleased || voice->egVolume < best->egVolume)) {
        best = voice;
      } else if (!released && !bestReleased && voice->start < best->start) {
        best = voice;
      }
    }
  }
  return best;
}

SynthVoice *CSynth::StealVoice(DWORD priority) {
  int         i;
  SynthVoice *sorted[SYNTH_VOICES + SYNTH_EXTRA_VOICES];
  int         count = 0;
  for (i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
    SynthVoice *voice = &m_voices[i];
    if (!voice->inUse || m_channels[voice->channel].priority > priority) {
      continue;
    }
    DWORD voicePriority = m_channels[voice->channel].priority;
    int   at = count++;
    while (at > 0 && (m_channels[sorted[at - 1]->channel].priority > voicePriority ||
                      (m_channels[sorted[at - 1]->channel].priority == voicePriority && sorted[at - 1]->order > voice->order)))
    {
      sorted[at] = sorted[at - 1];
      --at;
    }
    sorted[at] = voice;
  }

  SynthVoice *best = 0;
  for (i = 0; i < count; ++i) {
    SynthVoice *voice = sorted[i];
    if (!best) {
      best = voice;
    } else if (voice->egStop == SYNTH_FOREVER) {
      if (best->start > voice->start) {
        best = voice;
      }
    } else if (best->egStop == SYNTH_FOREVER || best->egVolume > voice->egVolume) {
      best = voice;
    }
  }
  if (best) {
    ClearVoice(best);
  }
  return best;
}

SynthVoice *CSynth::AllocVoice(int channel) {
  int active = 0;
  for (int i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
    if (m_voices[i].inUse) {
      ++active;
    }
  }
  if (active < SYNTH_VOICES + SYNTH_EXTRA_VOICES) {
    for (int i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
      if (!m_voices[i].inUse) {
        return &m_voices[i];
      }
    }
  }
  return StealVoice(m_channels[channel].priority);
}

void CSynth::NoteOn(LONGLONG time, int channelIndex, int key, int velocity) {
  DWORD         i;
  SynthChannel &channel = m_channels[channelIndex];

  if (channel.mono) {
    for (int i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
      if (m_voices[i].inUse && m_voices[i].channel == channelIndex) {
        StopVoice(&m_voices[i], time);
      }
    }
  }

  DWORD patch = channel.program | channel.bankLSB << 8 | channel.bankMSB << 16;
  if (channelIndex == 9) {
    patch |= SYNTH_DRUM_FLAG;
  }

  const SynthDownload *download = 0;
  for (i = 0; i < m_downloads.Count() && !download; ++i) {
    if (m_downloads[i].patch == patch) {
      download = &m_downloads[i];
    }
  }
  for (i = 0; i < m_downloads.Count() && !download && channelIndex == 9; ++i) {
    if (m_downloads[i].patch == SYNTH_DRUM_FLAG) {
      download = &m_downloads[i];
    }
  }
  if (!download) {
    return;
  }

  CSynthBank            *bank = download->bank;
  const SynthInstrument *instrument = download->instrument;
  DWORD                  serial = ++m_serial;

  for (DWORD r = 0; r < instrument->numRegions; ++r) {
    const SynthRegion *region = &bank->m_regions[instrument->firstRegion + r];
    if (key < region->keyLow || key > region->keyHigh || velocity < region->velocityLow || velocity > region->velocityHigh || region->wave == 0xFFFFFFFF) {
      continue;
    }

    if (!(region->options & 1)) {
      for (int i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
        SynthVoice *other = &m_voices[i];
        if (other->inUse && other->channel == channelIndex && other->key == key && other->region == region) {
          QuickStopVoice(other, time);
        }
      }
    }

    SynthVoice *voice = AllocVoice(channelIndex);
    if (!voice) {
      continue;
    }

    const SynthArticulation *art = region->hasArticulation ? &region->articulation : &instrument->articulation;
    const SynthWave         *wave = &bank->m_waves[region->wave];

    memset(voice, 0, sizeof(*voice));
    voice->inUse = true;
    voice->channel = channelIndex;
    voice->key = key;
    voice->serial = serial;
    voice->order = ++m_order;
    voice->bank = bank;
    bank->AddRef();
    voice->region = region;
    voice->wave = wave;
    voice->articulation = art;
    voice->start = time;
    voice->lastMix = time;

    LONGLONG minimum = SYNTH_SAMPLE_RATE / 1000;
    voice->egAttack = art->attack > minimum ? art->attack : minimum;
    voice->egRelease = art->release > minimum ? art->release : minimum;
    voice->egAttack = CentsToPitch(velocity * art->velocityToAttack / 127) * voice->egAttack / 4096;
    int keyScale = CentsToPitch(key * art->keyToDecay / 127);
    voice->egHold = keyScale * art->hold / 4096;
    voice->egDecay = (1000 - art->sustain) * (art->decay * keyScale / 4096) / 1000;
    voice->egDelay = art->delay;
    voice->egSustain = art->sustain;
    voice->egStop = SYNTH_FOREVER;
    voice->mixTime = voice->egDelay ? voice->egDelay : voice->egAttack;

    if (art->lfoModWheelToGain || art->lfoModWheelToPitch || art->lfoGainScale || art->lfoPitchScale) {
      voice->lfoInterval = 0x200000 / art->lfoFrequency;
    } else {
      voice->lfoInterval = 44100;
    }

    voice->base = s_midiToLevel[velocity] + region->attenuation;

    int tuning = channel.fineTune + (channelIndex == 9 ? 0 : channel.coarseTune);
    int cents = tuning + 100 * (key - region->unityNote) + region->fineTune;
    voice->basePitch = (DWORD)((ULONGLONG)(DWORD)CentsToPitch(cents) * wave->sampleRate / SYNTH_SAMPLE_RATE);
    voice->lastCents = (channel.bendRange * (GetControl(channel.bend, time) - 8192)) >> 13;
    voice->lastPitch = (DWORD)(((ULONGLONG)voice->basePitch * (DWORD)CentsToPitch(voice->lastCents)) >> 12);

    voice->looped = region->looped;
    voice->loopStart = region->loopStart;
    voice->loopEnd = region->loopEnd;
    voice->lastLevel[0] = -9600;
    voice->lastLevel[1] = -9600;
  }
}

void CSynth::NoteOff(LONGLONG time, int channelIndex, int key) {
  int         i;
  SynthVoice *first = 0;
  for (i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
    SynthVoice *voice = &m_voices[i];
    if (voice->inUse && voice->channel == channelIndex && voice->key == key && voice->egStop == SYNTH_FOREVER && !voice->sustained && !voice->tag) {
      if (!first || voice->order < first->order) {
        first = voice;
      }
    }
  }
  if (!first) {
    return;
  }

  DWORD serial = first->serial;
  for (i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
    SynthVoice *voice = &m_voices[i];
    if (voice->inUse && voice->serial == serial && voice->egStop == SYNTH_FOREVER && !voice->sustained) {
      if (m_channels[channelIndex].sustain) {
        voice->sustained = true;
      } else {
        StopVoice(voice, time);
      }
    }
  }
}

void CSynth::ProcessEvent(const SynthEvent &event) {
  int           i;
  int           key;
  int           index = event.status & 0x0F;
  SynthChannel &channel = m_channels[index];
  LONGLONG      time = event.time;

  switch (event.status & 0xF0) {
    case 0x80:
      NoteOff(time, index, event.data1);
      break;
    case 0x90:
      if (event.data2) {
        NoteOn(time, index, event.data1, event.data2);
      } else {
        NoteOff(time, index, event.data1);
      }
      break;
    case 0xC0:
      channel.program = event.data1;
      channel.bankMSB = channel.latchedMSB;
      channel.bankLSB = channel.latchedLSB;
      break;
    case 0xD0:
      Record(channel.pressure, time, event.data1);
      break;
    case 0xE0:
      Record(channel.bend, time, event.data1 | event.data2 << 7);
      break;
    case 0xB0:
      switch (event.data1) {
        case 0:
          channel.latchedMSB = event.data2;
          break;
        case 32:
          channel.latchedLSB = event.data2;
          break;
        case 1:
          Record(channel.modWheel, time, event.data2);
          break;
        case 7:
          Record(channel.volume, time, event.data2);
          break;
        case 10:
          Record(channel.pan, time, event.data2);
          break;
        case 11:
          Record(channel.expression, time, event.data2);
          break;
        case 6:
          channel.dataMSB = event.data2;
          if (channel.rpn == 0) {
            channel.bendRange = 100 * event.data2;
          } else if (channel.rpn == 1) {
            channel.fineTune = 100 * ((channel.dataMSB << 7 | channel.dataLSB) - 8192) / 8192;
          } else if (channel.rpn == 2) {
            channel.coarseTune = 100 * (event.data2 - 64);
          }
          break;
        case 38:
          channel.dataLSB = event.data2;
          if (channel.rpn == 1) {
            channel.fineTune = 100 * ((channel.dataMSB << 7 | channel.dataLSB) - 8192) / 8192;
          }
          break;
        case 98:
        case 99:
          channel.rpn = 0x3FFF;
          break;
        case 100:
          channel.rpn = (channel.rpn & 0x3F80) | event.data2;
          break;
        case 101:
          channel.rpn = (channel.rpn & 0x007F) | event.data2 << 7;
          break;
        case 64:
          channel.sustain = event.data2 != 0;
          if (!channel.sustain) {
            for (i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
              if (m_voices[i].inUse && m_voices[i].channel == index && m_voices[i].sustained) {
                m_voices[i].sustained = false;
                StopVoice(&m_voices[i], time);
              }
            }
          }
          break;
        case 121:
          Record(channel.expression, time, 127);
          Record(channel.bend, time, 0x2000);
          Record(channel.modWheel, time, 0);
          Record(channel.pressure, time, 0);
          if (event.data2) {
            Record(channel.volume, time, 100);
            Record(channel.pan, time, 64);
          }
          channel.sustain = false;
          for (i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
            if (m_voices[i].inUse && m_voices[i].channel == index && m_voices[i].sustained) {
              m_voices[i].sustained = false;
              StopVoice(&m_voices[i], time);
            }
          }
          break;
        case 123:
          for (key = 0; key < 128; ++key) {
            NoteOff(time, index, key);
          }
          break;
        case 120:
        case 126:
        case 127:
          if (event.data1 == 126) {
            channel.mono = true;
          } else if (event.data1 == 127) {
            channel.mono = false;
          }
          for (i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
            if (m_voices[i].inUse && m_voices[i].channel == index) {
              QuickStopVoice(&m_voices[i], time);
            }
          }
          break;
      }
      break;
  }
}

int CSynth::MixWave(SynthVoice *voice, short *left, short *right, int count, int delta, const int *volume, int cents) {
  DWORD pitch = (DWORD)(((ULONGLONG)voice->basePitch * (DWORD)CentsToPitch(cents)) >> 12);
  int   change = 2 * (cents > voice->lastCents ? cents - voice->lastCents : voice->lastCents - cents);
  if (change > delta) {
    delta = change;
  }
  delta >>= 1;
  voice->lastCents = cents;

  int step = delta ? 8 * count / delta : 512;
  if (step > 512) {
    step = 512;
  }
  if (step == 0) {
    step = 1;
  }
  step = (step + 3) & ~3;

  LONG pitchStep = MulDivRound((LONG)(pitch - voice->lastPitch), step << 8, count);
  LONG volumeStep[2];
  for (int k = 0; k < 2; ++k) {
    volumeStep[k] = MulDivRound(volume[k] - voice->lastVolume[k], step << 8, count);
  }

  const short *data = voice->wave->data;
  LONGLONG     end;
  LONGLONG     loopLength;
  if (voice->looped) {
    end = (LONGLONG)voice->loopEnd << 12;
    loopLength = (LONGLONG)(voice->loopEnd - voice->loopStart) << 12;
    if (loopLength <= (LONGLONG)pitch || loopLength > ((LONGLONG)voice->wave->numSamples << 12)) {
      return 0;
    }
  } else {
    end = (LONGLONG)voice->wave->numSamples << 12;
    loopLength = 0;
  }

  LONGLONG position = voice->position;
  LONG     pitchAccumulator = (LONG)(voice->lastPitch << 8);
  LONG     increment = (LONG)voice->lastPitch;
  LONG     volumeAccumulator[2] = {voice->lastVolume[0] << 8, voice->lastVolume[1] << 8};
  int      gain[2] = {voice->lastVolume[0], voice->lastVolume[1]};
  int      countdown = step;
  int      i = 0;

  while (i < count) {
    if (position >= end) {
      if (!loopLength) {
        break;
      }
      while (position >= end) {
        position -= loopLength;
      }
    }
    if (--countdown == 0) {
      countdown = step;
      pitchAccumulator += pitchStep;
      increment = pitchAccumulator >> 8;
      for (int k = 0; k < 2; ++k) {
        volumeAccumulator[k] += volumeStep[k];
        gain[k] = volumeAccumulator[k] >> 8;
      }
    }
    if (increment < 1) {
      increment = 1;
    }
    LONGLONG run = (increment - 1 + end - position) / increment;
    if (run > count - i) {
      run = count - i;
    }
    if (run > countdown) {
      run = countdown;
    }
    countdown = countdown + 1 - (int)run;
    while (run-- > 0) {
      LONG index = (LONG)(position >> 12);
      int  fraction = (int)(position & 0xFFF);
      int  sample = data[index] + (((data[index + 1] - data[index]) * fraction) >> 12);
      position += increment;
      left[i] = Saturate(left[i] + ((gain[0] * sample) >> 13));
      right[i] = Saturate(right[i] + ((gain[1] * sample) >> 13));
      ++i;
    }
  }

  voice->position = position;
  voice->lastPitch = pitch;
  voice->lastVolume[0] = volume[0];
  voice->lastVolume[1] = volume[1];
  return i;
}

void CSynth::MixVoice(SynthVoice *voice, short *left, short *right, LONGLONG start, LONGLONG end) {
  if (voice->lastMix >= end) {
    return;
  }

  SynthChannel            &channel = m_channels[voice->channel];
  const SynthArticulation *art = voice->articulation;
  LONGLONG                 position = voice->start;
  if (position < start) {
    position = start;
  }
  if (position < voice->lastMix) {
    position = voice->lastMix;
  }

  while (position < end) {
    if (voice->mixTime < 1) {
      voice->mixTime = 1;
    }
    LONGLONG segmentEnd = position + voice->mixTime;
    if (segmentEnd > end) {
      segmentEnd = end;
    }
    voice->mixTime = SYNTH_MAX_SPAN;
    if (voice->lastMix < voice->egStop && voice->egStop < end && voice->egStop - voice->lastMix < voice->mixTime) {
      voice->mixTime = voice->egStop - voice->lastMix;
    }

    LONGLONG next;
    int      lfo = LfoLevel(voice, segmentEnd, &next);
    if (next < voice->mixTime) {
      voice->mixTime = next;
    }
    int pressure = GetControl(channel.pressure, segmentEnd);
    int modWheel = GetControl(channel.modWheel, segmentEnd);
    int cents = (pressure * art->lfoPressureToPitch / 127 + modWheel * art->lfoModWheelToPitch / 127 + art->lfoPitchScale) * lfo / 100;
    cents += (channel.bendRange * (GetControl(channel.bend, segmentEnd) - 8192)) >> 13;

    int envelope = EnvelopeLevel(voice, segmentEnd, &next);
    if (next < voice->mixTime) {
      voice->mixTime = next;
    }
    int egLevel = (96 * envelope) / 10 - 9600;
    int tremolo = (pressure * art->lfoPressureToGain / 127 + modWheel * art->lfoModWheelToGain / 127 + art->lfoGainScale) * lfo / 100;
    int level = egLevel + tremolo + s_midiToLevel[GetControl(channel.expression, segmentEnd)] + s_midiToLevel[GetControl(channel.volume, segmentEnd)];
    int pan = art->pan + GetControl(channel.pan, segmentEnd);
    if (pan < 0) {
      pan = 0;
    }
    if (pan > 127) {
      pan = 127;
    }
    int bus[2] = {level + s_panToLevel[127 - pan], level + s_panToLevel[pan]};
    voice->egVolume = egLevel;
    bool finished = segmentEnd > voice->egStop && egLevel < -8000;

    int volume[2];
    int delta = 0;
    for (int k = 0; k < 2; ++k) {
      volume[k] = LevelToVolume(bus[k] + voice->base);
      int change = bus[k] > voice->lastLevel[k] ? bus[k] - voice->lastLevel[k] : voice->lastLevel[k] - bus[k];
      if (change > delta) {
        delta = change;
      }
      voice->lastLevel[k] = bus[k];
    }

    int count = (int)(segmentEnd - position);
    int offset = (int)(position - start);
    int produced = MixWave(voice, left + offset, right + offset, count, delta, volume, cents);
    position = segmentEnd;
    if (produced < count || finished) {
      ClearVoice(voice);
      return;
    }
  }

  voice->lastMix = position;
}

void CSynth::Mix(short *left, short *right, int count) {
  int      i;
  LONGLONG start = m_time;
  LONGLONG end = m_time + count;

  int active = 0;
  int tagged = 0;
  for (i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
    if (m_voices[i].inUse) {
      ++active;
      if (m_voices[i].tag) {
        ++tagged;
      }
    }
  }
  for (int owed = active - SYNTH_VOICES - tagged; owed > 0; --owed) {
    SynthVoice *voice = OldestVoice();
    if (!voice) {
      break;
    }
    QuickStopVoice(voice, end);
  }

  memset(left, 0, count * sizeof(short));
  memset(right, 0, count * sizeof(short));

  DWORD processed = 0;
  while (processed < m_events.Count() && m_events[processed].time < end) {
    SynthEvent event = m_events[processed++];
    if (event.time < start) {
      event.time = start;
    }
    ProcessEvent(event);
  }
  if (processed) {
    DWORD remaining = m_events.Count() - processed;
    for (DWORD i = 0; i < remaining; ++i) {
      m_events[i] = m_events[i + processed];
    }
    m_events.SetCount(remaining);
  }

  for (i = 0; i < SYNTH_VOICES + SYNTH_EXTRA_VOICES; ++i) {
    if (m_voices[i].inUse) {
      MixVoice(&m_voices[i], left, right, start, end);
    }
  }

  for (i = 0; i < SYNTH_CHANNELS; ++i) {
    FlushControl(m_channels[i].modWheel);
    FlushControl(m_channels[i].volume);
    FlushControl(m_channels[i].pan);
    FlushControl(m_channels[i].expression);
    FlushControl(m_channels[i].bend);
    FlushControl(m_channels[i].pressure);
  }

  for (i = 0; i < count; ++i) {
    left[i] = Saturate(left[i] * 2);
    right[i] = Saturate(right[i] * 2);
  }

  m_time = end;
}
