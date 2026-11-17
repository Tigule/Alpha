#pragma once

#include "Os/Mac/LoaderThread.h"
#include "Os/Mac/MP3Player.h"
#include "Os/Mac/WAVPlayer.h"

class Recycler {
 public:
  template <Player::PlayerType type>
  StreamPlayer *New();
  void          Delete(StreamPlayer *player);

  static Recycler s_singleton;

  TSGrowableArray<StreamPlayer *> m_wavPlayers;
  TSGrowableArray<StreamPlayer *> m_mp3Players;
};

template <>
inline StreamPlayer *Recycler::New<Player::e_wavPlayer>() {
  UINT count = m_wavPlayers.Count();
  if (count) {
    StreamPlayer *player = m_wavPlayers[count - 1];
    m_wavPlayers.SetCount(count - 1);
    return player;
  }
  return new WAVPlayer;
}

template <>
inline StreamPlayer *Recycler::New<Player::e_mp3Player>() {
  UINT count = m_mp3Players.Count();
  if (count) {
    StreamPlayer *player = m_mp3Players[count - 1];
    m_mp3Players.SetCount(count - 1);
    return player;
  }
  return new MP3Player;
}
