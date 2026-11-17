#include <Base/Base.h>

#include <storm.h>

#include "Os/Mac/Recycler.h"

Recycler Recycler::s_singleton;

void Recycler::Delete(StreamPlayer *player) {
  switch (player->Identify()) {
    case Player::e_wavPlayer:
      if (m_wavPlayers.Count() < 4) {
        player->Reset();
        *m_wavPlayers.New() = player;
      } else {
        player->Reset();
        player->SetStreamState(StreamPlayer::e_dead);
      }
      break;

    case Player::e_mp3Player:
      if (m_mp3Players.Count() < 1) {
        player->Reset();
        *m_mp3Players.New() = player;
      } else {
        player->Reset();
        player->SetStreamState(StreamPlayer::e_dead);
      }
      break;

    case Player::e_midiPlayer:
      player->Reset();
      player->SetStreamState(StreamPlayer::e_dead);
      break;

    default:
      SErrPrepareAppFatal(__FILE__, __LINE__);
      SErrDisplayAppFatal("unknown Player type %d", player->Identify());
      break;
  }
}
