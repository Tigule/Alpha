#include <Base/Base.h>

#include "Os/W32/OsSound.h"

#include <storm.h>

BYTE (*Sound::m_positionUpdateCallback)(LONGLONG handle, NTempest::C3Vector &position);

int Sound::Initialize(bool (*getIntCVar)(LPCSTR, int &), bool (*getFloatCVar)(LPCSTR, float &), bool (*getStringCVar)(LPCSTR, LPCSTR &)) {
  return 0;
}

void Sound::Shutdown() {
}

void Sound::SetSoundVolume(float volume) {
}

void Sound::SetMusicVolume(float volume) {
}

void Sound::SetMasterVolume(float volume) {
}

void Sound::MuteSFX(bool m) {
}

int Sound::MIDI_Initialize() {
  return 0;
}

void Sound::MIDI_Shutdown() {
}

void Sound::MIDI_Play(LPCSTR midiFilename, LPCSTR dlsFilename) {
}

void Sound::MIDI_Stop() {
}

void Sound::MIDI_SetVolume(float volume) {
}

void Sound::GetListenerPosition(NTempest::C3Vector &position) {
  position = NTempest::C3Vector(0.0f, 0.0f, 0.0f);
}

void Sound::SetListenerAttributes(
    const NTempest::C3Vector &position,
    const NTempest::C3Vector *velocity,
    const NTempest::C3Vector &forward,
    const NTempest::C3Vector &up
) {
}

Sound *Sound::Play2D(SOUNDCATEGORIES category, LPCSTR filename, int flags, bool startPaused) {
  return 0;
}

Sound *Sound::Play3D(SOUNDCATEGORIES category, LPCSTR filename, int flags, bool startPaused) {
  return 0;
}

Sound *Sound::Play2DLooped(SOUNDCATEGORIES category, LPCSTR filename, int flags, UINT loopCount, bool startPaused) {
  return 0;
}

Sound *Sound::Play3DLooped(SOUNDCATEGORIES category, LPCSTR filename, int flags, UINT loopCount, bool startPaused) {
  return 0;
}

void Sound::KillSound(Sound *&sound) {
  sound = 0;
}

void Sound::SetReverbProperties(const _FSOUND_REVERB_PROPERTIES *reverb) {
}

void Sound::Stop(float fadeTime) {
}

void Sound::SetFadeIn(float fadeTime, float volume) {
}

bool Sound::IsPlaying() {
  return false;
}

bool Sound::IsOutOfRange() {
  return false;
}

void Sound::Set3DUpdateHandle(LONGLONG handle) {
}

bool Sound::SetPaused(bool state) {
  return false;
}

void Sound::SetPosition(const NTempest::C3Vector &worldPosition, const NTempest::C3Vector *vel) {
}

void Sound::SetReverbProperties(const _FSOUND_REVERB_CHANNELPROPERTIES *reverb) {
}

void Sound::SetPanning(float pan) {
}

void Sound::SetCutoffDistanceSquared(float distanceSquared) {
}

void Sound::SetFrequency(int freq) {
}

void Sound::SetDistances(float min, float max) {
}

void Sound::SetVolume(float volume) {
}

int SndGetCPUPerformance() {
  return 0;
}
