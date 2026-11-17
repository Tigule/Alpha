#pragma once

class Player {
 public:
  enum PlayerType {
    e_wavPlayer = 0,
    e_mp3Player = 1,
    e_midiPlayer = 2
  };

  virtual ~Player() {
  }
  virtual void        Start() = 0;
  virtual void        Stop() = 0;
  virtual void        Reset() = 0;
  virtual void        SetVolume(float volume) = 0;
  virtual bool        IsPlaying() = 0;
  virtual PlayerType  Identify() = 0;
  virtual const char *GetSource() = 0;
  virtual void        SetLoopCount(UINT loopCount) {
  }
  virtual void Set3DPositionSpherical(float distance, float azimuth, float elevation) {
  }
  virtual void ReadAhead() = 0;
};

class StreamPlayer : public Player {
 public:
  enum StreamState {
    e_inactive = 0,
    e_active = 1,
    e_dead = 2
  };

  StreamPlayer();
  virtual ~StreamPlayer();
  void SetStreamState(StreamState state);

  StreamPlayer *volatile m_next;
  volatile LONG          m_state;
};

void LoaderThreadInitialize();
void LoaderThreadCleanUp();
