#include <stdint.h>

#define WINAPI

struct FSOUND_SAMPLE;
struct FSOUND_STREAM;

typedef signed char(WINAPI *FSOUND_STREAMCALLBACK)(FSOUND_STREAM *, void *, int, intptr_t);
typedef void *(WINAPI *FSOUND_ALLOCCALLBACK)(unsigned int);
typedef void *(WINAPI *FSOUND_REALLOCCALLBACK)(void *, unsigned int);
typedef void(WINAPI *FSOUND_FREECALLBACK)(void *);
typedef unsigned int(WINAPI *FSOUND_OPENCALLBACK)(const char *);
typedef void(WINAPI *FSOUND_CLOSECALLBACK)(unsigned int);
typedef int(WINAPI *FSOUND_READCALLBACK)(void *, int, unsigned int);
typedef int(WINAPI *FSOUND_SEEKCALLBACK)(unsigned int, int, signed char);
typedef int(WINAPI *FSOUND_TELLCALLBACK)(unsigned int);

enum {
  FMOD_ERR_INIT = 3
};

extern "C" {

signed char WINAPI FSOUND_SetOutput(int outputtype) {
  return 1;
}

int WINAPI FSOUND_GetOutput() {
  return 0;
}

int WINAPI FSOUND_GetNumDrivers() {
  return 0;
}

const char *WINAPI FSOUND_GetDriverName(int driver) {
  return "";
}

signed char WINAPI FSOUND_SetDriver(int driver) {
  return 1;
}

int WINAPI FSOUND_GetDriver() {
  return 0;
}

signed char WINAPI FSOUND_GetDriverCaps(int driver, unsigned int *caps) {
  return 0;
}

signed char WINAPI FSOUND_SetMixer(int mixer) {
  return 1;
}

int WINAPI FSOUND_GetMixer() {
  return 0;
}

signed char WINAPI FSOUND_SetBufferSize(int milliseconds) {
  return 1;
}

signed char WINAPI FSOUND_SetMinHardwareChannels(int minimum) {
  return 1;
}

signed char WINAPI FSOUND_SetMaxHardwareChannels(int maximum) {
  return 1;
}

signed char WINAPI FSOUND_SetMemorySystem(
    void                  *pool,
    int                    poolLength,
    FSOUND_ALLOCCALLBACK   allocCallback,
    FSOUND_REALLOCCALLBACK reallocCallback,
    FSOUND_FREECALLBACK    freeCallback
) {
  return 1;
}

void WINAPI FSOUND_File_SetCallbacks(
    FSOUND_OPENCALLBACK  openCallback,
    FSOUND_CLOSECALLBACK closeCallback,
    FSOUND_READCALLBACK  readCallback,
    FSOUND_SEEKCALLBACK  seekCallback,
    FSOUND_TELLCALLBACK  tellCallback
) {
}

void WINAPI FSOUND_3D_SetDistanceFactor(float factor) {
}

void WINAPI FSOUND_3D_SetDopplerFactor(float factor) {
}

void WINAPI FSOUND_3D_SetRolloffFactor(float factor) {
}

signed char WINAPI FSOUND_Init(int mixrate, int maxsoftwarechannels, unsigned int flags) {
  return 0;
}

void WINAPI FSOUND_Close() {
}

void WINAPI FSOUND_Update() {
}

int WINAPI FSOUND_GetError() {
  return FMOD_ERR_INIT;
}

int WINAPI FSOUND_GetMaxChannels() {
  return 0;
}

int WINAPI FSOUND_GetNumHardwareChannels() {
  return 0;
}

int WINAPI FSOUND_GetOutputRate() {
  return 0;
}

float WINAPI FSOUND_GetCPUUsage() {
  return 0.0f;
}

void WINAPI FSOUND_SetSFXMasterVolume(int volume) {
}

signed char WINAPI FSOUND_3D_Listener_SetAttributes(
    const float *position,
    const float *velocity,
    float        forwardX,
    float        forwardY,
    float        forwardZ,
    float        topX,
    float        topY,
    float        topZ
) {
  return 0;
}

void WINAPI FSOUND_3D_Listener_GetAttributes(
    float *position,
    float *velocity,
    float *forwardX,
    float *forwardY,
    float *forwardZ,
    float *topX,
    float *topY,
    float *topZ
) {
}

FSOUND_STREAM *WINAPI FSOUND_Stream_Open(const char *filename, unsigned int mode, int offset, int length) {
  return 0;
}

signed char WINAPI FSOUND_Stream_Close(FSOUND_STREAM *stream) {
  return 0;
}

int WINAPI FSOUND_Stream_PlayEx(int channel, FSOUND_STREAM *stream, void *dsp, signed char startpaused) {
  return -1;
}

signed char WINAPI FSOUND_Stream_Stop(FSOUND_STREAM *stream) {
  return 0;
}

signed char WINAPI FSOUND_Stream_SetEndCallback(FSOUND_STREAM *stream, FSOUND_STREAMCALLBACK callback, intptr_t userdata) {
  return 0;
}

signed char WINAPI FSOUND_Stream_SetLoopCount(FSOUND_STREAM *stream, int count) {
  return 0;
}

int WINAPI FSOUND_Stream_GetLengthMs(FSOUND_STREAM *stream) {
  return 0;
}

signed char WINAPI FSOUND_Stream_SetTime(FSOUND_STREAM *stream, int milliseconds) {
  return 0;
}

FSOUND_SAMPLE *WINAPI FSOUND_Stream_GetSample(FSOUND_STREAM *stream) {
  return 0;
}

signed char WINAPI FSOUND_Sample_SetMinMaxDistance(FSOUND_SAMPLE *sample, float minDistance, float maxDistance) {
  return 0;
}

signed char WINAPI FSOUND_StopSound(int channel) {
  return 0;
}

signed char WINAPI FSOUND_IsPlaying(int channel) {
  return 0;
}

signed char WINAPI FSOUND_SetPaused(int channel, signed char paused) {
  return 0;
}

signed char WINAPI FSOUND_SetMute(int channel, signed char mute) {
  return 0;
}

signed char WINAPI FSOUND_SetVolume(int channel, int volume) {
  return 0;
}

signed char WINAPI FSOUND_SetFrequency(int channel, int frequency) {
  return 0;
}

signed char WINAPI FSOUND_3D_SetAttributes(int channel, const float *position, const float *velocity) {
  return 0;
}
}
