#pragma once

#include <stpl.h>

#define SYNTH_SAMPLE_RATE  22050
#define SYNTH_VOICES       64
#define SYNTH_EXTRA_VOICES 16
#define SYNTH_CHANNELS     16
#define SYNTH_DRUM_FLAG    0x80000000

struct SynthWave {
  short *data;
  DWORD  numSamples;
  DWORD  sampleRate;
  BOOL   hasSample;
  WORD   unityNote;
  short  fineTune;
  LONG   attenuation;
  DWORD  loopCount;
  DWORD  loopType;
  DWORD  loopStart;
  DWORD  loopLength;
};

struct SynthArticulation {
  LONGLONG attack;
  LONGLONG decay;
  LONGLONG release;
  LONGLONG delay;
  LONGLONG hold;
  short    sustain;
  short    velocityToAttack;
  short    keyToDecay;
  short    keyToHold;
  DWORD    lfoFrequency;
  LONGLONG lfoDelay;
  short    lfoModWheelToGain;
  short    lfoModWheelToPitch;
  short    lfoGainScale;
  short    lfoPitchScale;
  short    lfoPressureToGain;
  short    lfoPressureToPitch;
  short    pan;
};

struct SynthRegion {
  WORD              keyLow;
  WORD              keyHigh;
  WORD              velocityLow;
  WORD              velocityHigh;
  WORD              options;
  DWORD             wave;
  WORD              unityNote;
  short             fineTune;
  int               attenuation;
  BOOL              looped;
  DWORD             loopType;
  DWORD             loopStart;
  DWORD             loopEnd;
  BOOL              hasArticulation;
  SynthArticulation articulation;
};

struct SynthInstrument {
  DWORD             patch;
  DWORD             firstRegion;
  DWORD             numRegions;
  SynthArticulation articulation;
};

class CSynthBank {
 public:
  CSynthBank();
  ~CSynthBank();
  bool                   Load(const BYTE *data, DWORD size);
  const SynthInstrument *Find(DWORD patch) const;
  void                   AddRef();
  void                   Release();

  TSGrowableArray<SynthWave>       m_waves;
  TSGrowableArray<SynthRegion>     m_regions;
  TSGrowableArray<SynthInstrument> m_instruments;

 private:
  int m_refs;
};

struct SynthControl {
  LONGLONG time;
  int      value;
};

struct SynthRecorder {
  int                           value;
  TSGrowableArray<SynthControl> events;
};

struct SynthChannel {
  SynthRecorder modWheel;
  SynthRecorder volume;
  SynthRecorder pan;
  SynthRecorder expression;
  SynthRecorder bend;
  SynthRecorder pressure;
  int           program;
  int           bankMSB;
  int           bankLSB;
  int           latchedMSB;
  int           latchedLSB;
  int           bendRange;
  int           fineTune;
  int           coarseTune;
  int           rpn;
  int           dataMSB;
  int           dataLSB;
  bool          sustain;
  bool          mono;
  DWORD         priority;
};

struct SynthVoice {
  bool                     inUse;
  bool                     tag;
  bool                     sustained;
  int                      channel;
  int                      key;
  DWORD                    serial;
  DWORD                    order;
  CSynthBank              *bank;
  const SynthRegion       *region;
  const SynthWave         *wave;
  LONGLONG                 start;
  LONGLONG                 lastMix;
  LONGLONG                 mixTime;
  LONGLONG                 egAttack;
  LONGLONG                 egDecay;
  LONGLONG                 egRelease;
  LONGLONG                 egDelay;
  LONGLONG                 egHold;
  LONGLONG                 egStop;
  int                      egSustain;
  const SynthArticulation *articulation;
  LONGLONG                 lfoInterval;
  int                      base;
  DWORD                    basePitch;
  BOOL                     looped;
  DWORD                    loopStart;
  DWORD                    loopEnd;
  LONGLONG                 position;
  DWORD                    lastPitch;
  int                      lastCents;
  int                      lastVolume[2];
  int                      lastLevel[2];
  int                      egVolume;
};

struct SynthEvent {
  LONGLONG time;
  BYTE     status;
  BYTE     data1;
  BYTE     data2;
};

struct SynthDownload {
  DWORD                  patch;
  CSynthBank            *bank;
  const SynthInstrument *instrument;
};

class CSynth {
 public:
  CSynth();
  ~CSynth();
  void     Download(CSynthBank *bank, DWORD patch);
  void     UnloadAll();
  void     PlayMessage(LONGLONG time, BYTE status, BYTE data1, BYTE data2);
  void     Mix(short *left, short *right, int count);
  LONGLONG Time() const;

 private:
  void        ProcessEvent(const SynthEvent &event);
  void        NoteOn(LONGLONG time, int channel, int key, int velocity);
  void        NoteOff(LONGLONG time, int channel, int key);
  void        StopVoice(SynthVoice *voice, LONGLONG time);
  void        QuickStopVoice(SynthVoice *voice, LONGLONG time);
  void        ClearVoice(SynthVoice *voice);
  SynthVoice *AllocVoice(int channel);
  SynthVoice *OldestVoice();
  SynthVoice *StealVoice(DWORD priority);
  void        MixVoice(SynthVoice *voice, short *left, short *right, LONGLONG start, LONGLONG end);
  int         MixWave(SynthVoice *voice, short *left, short *right, int count, int delta, const int *volume, int cents);

  SynthChannel                   m_channels[SYNTH_CHANNELS];
  SynthVoice                     m_voices[SYNTH_VOICES + SYNTH_EXTRA_VOICES];
  TSGrowableArray<SynthEvent>    m_events;
  TSGrowableArray<SynthDownload> m_downloads;
  LONGLONG                       m_time;
  DWORD                          m_serial;
  DWORD                          m_order;
};
