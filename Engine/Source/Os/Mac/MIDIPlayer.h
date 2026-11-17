#pragma once

#include "Os/Mac/LoaderThread.h"

#include <AudioToolbox/AudioToolbox.h>

class MIDIPlayer : public StreamPlayer {
 public:
  MIDIPlayer();
  virtual ~MIDIPlayer();
  virtual void        Start();
  virtual void        Stop();
  virtual void        Reset();
  virtual void        SetVolume(float volume);
  virtual bool        IsPlaying();
  virtual PlayerType  Identify();
  virtual const char *GetSource();
  virtual void        ReadAhead();
  const char         *GetSourceDLS();
  bool                SetMIDISource(const char *name);
  bool                SetDLSSource(const char *name);

 private:
  void InitializeIfReady();
  void Dispose();

  CFURLRef    m_dlsURL;
  CFURLRef    m_midiURL;
  AUNode      m_synthNode;
  AudioUnit   m_synthUnit;
  MusicSequence m_sequence;
  MusicPlayer m_player;
  DWORD       m_startTicks;
  float       m_volume;
  bool        m_connected;
  char        m_midiName[260];
  char        m_dlsName[260];
};
