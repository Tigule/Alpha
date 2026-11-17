#include <AL/al.h>
#include <AL/alc.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct FSOUND_SAMPLE {
  float minDistance;
  float maxDistance;
};

struct FSOUND_STREAM;

typedef signed char (*FSOUND_STREAMCALLBACK)(FSOUND_STREAM *, void *, int, intptr_t);
typedef void *(*FSOUND_ALLOCCALLBACK)(unsigned int);
typedef void *(*FSOUND_REALLOCCALLBACK)(void *, unsigned int);
typedef void (*FSOUND_FREECALLBACK)(void *);
typedef unsigned int (*FSOUND_OPENCALLBACK)(const char *);
typedef void (*FSOUND_CLOSECALLBACK)(unsigned int);
typedef int (*FSOUND_READCALLBACK)(void *, int, unsigned int);
typedef int (*FSOUND_SEEKCALLBACK)(unsigned int, int, signed char);
typedef int (*FSOUND_TELLCALLBACK)(unsigned int);

struct FSOUND_STREAM {
  FSOUND_SAMPLE         sample;
  unsigned int          file;
  unsigned int          mode;
  int                   channel;
  int                   loopCount;
  int                   loopsLeft;
  ALenum                format;
  int                   rate;
  int                   blockAlign;
  int                   dataStart;
  int                   dataSize;
  int                   position;
  bool                  ended;
  FSOUND_STREAMCALLBACK endCallback;
  intptr_t              endUserData;
  int                   pcmSize;
  unsigned char        *pcm;
};

enum {
  FMOD_ERR_NONE = 0,
  FMOD_ERR_BUSY = 1,
  FMOD_ERR_UNINITIALIZED = 2,
  FMOD_ERR_INIT = 3,
  FMOD_ERR_FILE_NOTFOUND = 9,
  FMOD_ERR_FILE_FORMAT = 10,
  FMOD_ERR_MEMORY = 12,
  FMOD_ERR_INVALID_PARAM = 14,
  FMOD_ERR_CHANNEL_ALLOC = 16
};

enum {
  FSOUND_OUTPUT_NOSOUND = 0,
  FSOUND_OUTPUT_OSS = 4,
  FSOUND_OUTPUT_ESD = 5,
  FSOUND_OUTPUT_ALSA = 6,
  FSOUND_OUTPUT_NOSOUND_NONREALTIME = 12
};

enum {
  FSOUND_MIXER_AUTODETECT = 0,
  FSOUND_MIXER_BLENDMODE = 1,
  FSOUND_MIXER_QUALITY_AUTODETECT = 4,
  FSOUND_MIXER_QUALITY_FPU = 5,
  FSOUND_MIXER_MAX = 10
};

enum {
  FSOUND_LOOP_NORMAL = 0x2,
  FSOUND_2D = 0x2000,
  FSOUND_FREE = -1,
  FSOUND_ALL = -3
};

enum {
  NUM_BUFFERS = 4,
  BUFFERS_PER_SECOND = 20,
  STREAM_THREAD_MS = 10,
  MAX_DRIVERS = 32
};

struct Channel {
  ALuint         source;
  ALuint         buffers[NUM_BUFFERS];
  FSOUND_STREAM *stream;
  int            ref;
  int            volume;
  bool           paused;
  bool           muted;
};

static void *DefaultAlloc(unsigned int size) {
  return malloc(size);
}

static void DefaultFree(void *ptr) {
  free(ptr);
}

static pthread_mutex_t       s_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t             s_thread;
static volatile bool         s_running;
static bool                  s_initialized;
static ALCdevice            *s_device;
static ALCcontext           *s_context;
static Channel              *s_channels;
static int                   s_numChannels;
static int                   s_error;
static int                   s_output = -1;
static int                   s_driver;
static int                   s_mixer = FSOUND_MIXER_QUALITY_AUTODETECT;
static int                   s_mixRate = 44100;
static int                   s_masterVolume = 255;
static float                 s_distanceFactor = 1.0f;
static float                 s_dopplerFactor = 1.0f;
static float                 s_rolloffFactor = 1.0f;
static float                 s_listenerPos[3];
static float                 s_listenerVel[3];
static float                 s_listenerForward[3] = {0.0f, 0.0f, 1.0f};
static float                 s_listenerTop[3] = {0.0f, 1.0f, 0.0f};
static FSOUND_ALLOCCALLBACK  s_alloc = DefaultAlloc;
static FSOUND_FREECALLBACK   s_free = DefaultFree;
static FSOUND_OPENCALLBACK   s_open;
static FSOUND_CLOSECALLBACK  s_close;
static FSOUND_READCALLBACK   s_read;
static FSOUND_SEEKCALLBACK   s_seek;
static FSOUND_TELLCALLBACK   s_tell;
static int                   s_numDrivers = -1;
static const char           *s_driverNames[MAX_DRIVERS];
static char                  s_driverNameData[4096];
static int                   s_driverNameUsed;

static void AddDriver(const char *name) {
  int length = strlen(name) + 1;

  if (s_numDrivers >= MAX_DRIVERS || s_driverNameUsed + length > (int)sizeof(s_driverNameData)) {
    return;
  }

  memcpy(s_driverNameData + s_driverNameUsed, name, length);
  s_driverNames[s_numDrivers++] = s_driverNameData + s_driverNameUsed;
  s_driverNameUsed += length;
}

static void EnumerateDrivers() {
  if (s_numDrivers >= 0) {
    return;
  }

  s_numDrivers = 0;
  bool        all = alcIsExtensionPresent(0, "ALC_ENUMERATE_ALL_EXT") == ALC_TRUE;
  const char *name = alcGetString(0, all ? ALC_DEFAULT_ALL_DEVICES_SPECIFIER : ALC_DEFAULT_DEVICE_SPECIFIER);
  if (name && *name) {
    AddDriver(name);
  }

  for (name = alcGetString(0, all ? ALC_ALL_DEVICES_SPECIFIER : ALC_DEVICE_SPECIFIER); name && *name; name += strlen(name) + 1) {
    if (!s_numDrivers || strcmp(name, s_driverNames[0])) {
      AddDriver(name);
    }
  }
}

static void ApplyListener() {
  ALfloat orientation[6] = {s_listenerForward[0], s_listenerForward[1], -s_listenerForward[2], s_listenerTop[0], s_listenerTop[1], -s_listenerTop[2]};

  alListener3f(AL_POSITION, s_listenerPos[0], s_listenerPos[1], -s_listenerPos[2]);
  alListener3f(AL_VELOCITY, s_listenerVel[0], s_listenerVel[1], -s_listenerVel[2]);
  alListenerfv(AL_ORIENTATION, orientation);
}

static Channel *GetChannel(int handle) {
  int index = handle & 0xFFFF;

  if (handle < 0 || index >= s_numChannels || !s_channels[index].stream || s_channels[index].ref != (handle >> 16)) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  return &s_channels[index];
}

static void ApplyGain(Channel *channel) {
  alSourcef(channel->source, AL_GAIN, channel->muted ? 0.0f : channel->volume / 255.0f);
}

static bool FillBuffer(FSOUND_STREAM *stream, ALuint buffer) {
  int filled = 0;

  while (filled < stream->pcmSize) {
    if (stream->position >= stream->dataSize) {
      if (!(stream->mode & FSOUND_LOOP_NORMAL) || !stream->loopsLeft || !stream->dataSize) {
        break;
      }
      if (stream->loopsLeft > 0) {
        --stream->loopsLeft;
      }
      s_seek(stream->file, stream->dataStart, 0);
      stream->position = 0;
    }

    int size = stream->pcmSize - filled;
    if (size > stream->dataSize - stream->position) {
      size = stream->dataSize - stream->position;
    }

    int bytesRead = s_read(stream->pcm + filled, size, stream->file);
    if (bytesRead <= 0) {
      stream->loopsLeft = 0;
      stream->position = stream->dataSize;
      break;
    }

    filled += bytesRead;
    stream->position += bytesRead;
  }

  filled -= filled % stream->blockAlign;
  if (!filled) {
    stream->ended = true;
    return false;
  }

  alBufferData(buffer, stream->format, stream->pcm, filled, stream->rate);
  return true;
}

static void QueueBuffers(Channel *channel) {
  for (int i = 0; i < NUM_BUFFERS && FillBuffer(channel->stream, channel->buffers[i]); ++i) {
    alSourceQueueBuffers(channel->source, 1, &channel->buffers[i]);
  }

  if (!channel->paused) {
    alSourcePlay(channel->source);
  }
}

static void ReleaseChannel(Channel *channel) {
  alSourceRewind(channel->source);
  alSourcei(channel->source, AL_BUFFER, 0);
  channel->stream->channel = -1;
  channel->stream = 0;
  channel->ref = channel->ref % 0x7FFF + 1;
}

static void StopStream(FSOUND_STREAM *stream) {
  if (stream->channel != -1) {
    ReleaseChannel(&s_channels[stream->channel & 0xFFFF]);
  }

  s_seek(stream->file, stream->dataStart, 0);
  stream->position = 0;
  stream->loopsLeft = stream->loopCount;
  stream->ended = false;
}

static void *StreamThread(void *) {
  while (s_running) {
    pthread_mutex_lock(&s_lock);

    for (int i = 0; i < s_numChannels; ++i) {
      Channel       *channel = &s_channels[i];
      FSOUND_STREAM *stream = channel->stream;
      ALint          processed = 0;
      ALint          queued = 0;
      ALint          state = 0;

      if (!stream) {
        continue;
      }

      alGetSourcei(channel->source, AL_BUFFERS_PROCESSED, &processed);
      while (processed-- > 0) {
        ALuint buffer;
        alSourceUnqueueBuffers(channel->source, 1, &buffer);
        if (!stream->ended && FillBuffer(stream, buffer)) {
          alSourceQueueBuffers(channel->source, 1, &buffer);
        }
      }

      alGetSourcei(channel->source, AL_BUFFERS_QUEUED, &queued);
      if (!queued) {
        ReleaseChannel(channel);
        if (stream->endCallback) {
          stream->endCallback(stream, 0, 0, stream->endUserData);
        }
        continue;
      }

      alGetSourcei(channel->source, AL_SOURCE_STATE, &state);
      if (state != AL_PLAYING && !channel->paused) {
        alSourcePlay(channel->source);
      }
    }

    pthread_mutex_unlock(&s_lock);

    struct timespec delay = {0, STREAM_THREAD_MS * 1000000};
    nanosleep(&delay, 0);
  }

  return 0;
}

extern "C" {

signed char FSOUND_SetOutput(int outputtype) {
  if (s_initialized) {
    s_error = FMOD_ERR_BUSY;
    return 0;
  }

  if (outputtype != -1 && outputtype != FSOUND_OUTPUT_NOSOUND && outputtype != FSOUND_OUTPUT_OSS && outputtype != FSOUND_OUTPUT_ESD &&
      outputtype != FSOUND_OUTPUT_ALSA && outputtype != FSOUND_OUTPUT_NOSOUND_NONREALTIME) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  s_output = outputtype;
  return 1;
}

int FSOUND_GetOutput() {
  return s_output == -1 ? FSOUND_OUTPUT_ALSA : s_output;
}

int FSOUND_GetNumDrivers() {
  EnumerateDrivers();
  return s_numDrivers;
}

const char *FSOUND_GetDriverName(int driver) {
  EnumerateDrivers();

  if (driver < 0 || driver >= s_numDrivers) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return "";
  }

  return s_driverNames[driver];
}

signed char FSOUND_SetDriver(int driver) {
  if (s_initialized) {
    s_error = FMOD_ERR_BUSY;
    return 0;
  }

  if (driver < 0) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  s_driver = driver;
  return 1;
}

int FSOUND_GetDriver() {
  return s_driver;
}

signed char FSOUND_GetDriverCaps(int driver, unsigned int *caps) {
  *caps = 0;
  return 1;
}

signed char FSOUND_SetMixer(int mixer) {
  if (mixer < 0 || mixer >= FSOUND_MIXER_MAX) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  s_mixer = mixer;
  return 1;
}

int FSOUND_GetMixer() {
  if (s_mixer == FSOUND_MIXER_AUTODETECT) {
    return FSOUND_MIXER_BLENDMODE;
  }

  if (s_mixer == FSOUND_MIXER_QUALITY_AUTODETECT) {
    return FSOUND_MIXER_QUALITY_FPU;
  }

  return s_mixer;
}

signed char FSOUND_SetBufferSize(int milliseconds) {
  if (s_initialized) {
    s_error = FMOD_ERR_BUSY;
    return 0;
  }

  return 1;
}

signed char FSOUND_SetMinHardwareChannels(int minimum) {
  return 1;
}

signed char FSOUND_SetMaxHardwareChannels(int maximum) {
  return 1;
}

signed char FSOUND_SetMemorySystem(
    void                  *pool,
    int                    poolLength,
    FSOUND_ALLOCCALLBACK   allocCallback,
    FSOUND_REALLOCCALLBACK reallocCallback,
    FSOUND_FREECALLBACK    freeCallback
) {
  if (s_initialized) {
    s_error = FMOD_ERR_BUSY;
    return 0;
  }

  if (pool || !allocCallback || !freeCallback) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  s_alloc = allocCallback;
  s_free = freeCallback;
  return 1;
}

void FSOUND_File_SetCallbacks(
    FSOUND_OPENCALLBACK  openCallback,
    FSOUND_CLOSECALLBACK closeCallback,
    FSOUND_READCALLBACK  readCallback,
    FSOUND_SEEKCALLBACK  seekCallback,
    FSOUND_TELLCALLBACK  tellCallback
) {
  s_open = openCallback;
  s_close = closeCallback;
  s_read = readCallback;
  s_seek = seekCallback;
  s_tell = tellCallback;
}

void FSOUND_3D_SetDistanceFactor(float factor) {
  s_distanceFactor = factor;

  if (s_context) {
    alSpeedOfSound(343.3f * s_distanceFactor);
  }
}

void FSOUND_3D_SetDopplerFactor(float factor) {
  s_dopplerFactor = factor;

  if (s_context) {
    alDopplerFactor(s_dopplerFactor);
  }
}

void FSOUND_3D_SetRolloffFactor(float factor) {
  pthread_mutex_lock(&s_lock);

  s_rolloffFactor = factor;
  for (int i = 0; i < s_numChannels; ++i) {
    if (s_channels[i].stream && !(s_channels[i].stream->mode & FSOUND_2D)) {
      alSourcef(s_channels[i].source, AL_ROLLOFF_FACTOR, s_rolloffFactor);
    }
  }

  pthread_mutex_unlock(&s_lock);
}

void FSOUND_Close();

signed char FSOUND_Init(int mixrate, int maxsoftwarechannels, unsigned int flags) {
  if (s_initialized) {
    s_error = FMOD_ERR_BUSY;
    return 0;
  }

  if (mixrate < 4000 || mixrate > 65535 || maxsoftwarechannels < 0 || maxsoftwarechannels > 1024) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  s_mixRate = mixrate;
  s_initialized = true;

  if (s_output == FSOUND_OUTPUT_NOSOUND || s_output == FSOUND_OUTPUT_NOSOUND_NONREALTIME) {
    return 1;
  }

  EnumerateDrivers();
  s_device = alcOpenDevice(s_driver < s_numDrivers ? s_driverNames[s_driver] : 0);
  if (s_device) {
    ALCint attributes[] = {ALC_FREQUENCY, mixrate, 0};
    s_context = alcCreateContext(s_device, attributes);
  }

  if (!s_context || !alcMakeContextCurrent(s_context)) {
    FSOUND_Close();
    s_error = FMOD_ERR_INIT;
    return 0;
  }

  s_channels = (Channel *)s_alloc(maxsoftwarechannels * sizeof(Channel));
  if (!s_channels) {
    FSOUND_Close();
    s_error = FMOD_ERR_MEMORY;
    return 0;
  }

  alGetError();
  for (s_numChannels = 0; s_numChannels < maxsoftwarechannels; ++s_numChannels) {
    Channel *channel = &s_channels[s_numChannels];
    memset(channel, 0, sizeof(*channel));
    channel->ref = 1;

    alGenSources(1, &channel->source);
    if (alGetError() != AL_NO_ERROR) {
      break;
    }

    alGenBuffers(NUM_BUFFERS, channel->buffers);
    if (alGetError() != AL_NO_ERROR) {
      alDeleteSources(1, &channel->source);
      break;
    }
  }

  if (s_numChannels < maxsoftwarechannels) {
    FSOUND_Close();
    s_error = FMOD_ERR_INIT;
    return 0;
  }

  alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
  alSpeedOfSound(343.3f * s_distanceFactor);
  alDopplerFactor(s_dopplerFactor);
  alListenerf(AL_GAIN, s_masterVolume / 255.0f);
  ApplyListener();

  s_running = true;
  if (pthread_create(&s_thread, 0, StreamThread, 0)) {
    s_running = false;
    FSOUND_Close();
    s_error = FMOD_ERR_INIT;
    return 0;
  }

  return 1;
}

void FSOUND_Close() {
  if (!s_initialized) {
    return;
  }

  if (s_running) {
    s_running = false;
    pthread_join(s_thread, 0);
  }

  pthread_mutex_lock(&s_lock);

  for (int i = 0; i < s_numChannels; ++i) {
    if (s_channels[i].stream) {
      ReleaseChannel(&s_channels[i]);
    }
    alDeleteSources(1, &s_channels[i].source);
    alDeleteBuffers(NUM_BUFFERS, s_channels[i].buffers);
  }

  if (s_channels) {
    s_free(s_channels);
    s_channels = 0;
  }
  s_numChannels = 0;

  pthread_mutex_unlock(&s_lock);

  alcMakeContextCurrent(0);
  if (s_context) {
    alcDestroyContext(s_context);
    s_context = 0;
  }
  if (s_device) {
    alcCloseDevice(s_device);
    s_device = 0;
  }

  s_initialized = false;
}

void FSOUND_Update() {
}

int FSOUND_GetError() {
  return s_error;
}

int FSOUND_GetMaxChannels() {
  return s_numChannels;
}

int FSOUND_GetNumHardwareChannels() {
  return 0;
}

int FSOUND_GetOutputRate() {
  ALCint frequency = s_mixRate;

  if (s_device) {
    alcGetIntegerv(s_device, ALC_FREQUENCY, 1, &frequency);
  }

  return frequency;
}

float FSOUND_GetCPUUsage() {
  return 0.0f;
}

void FSOUND_SetSFXMasterVolume(int volume) {
  s_masterVolume = volume < 0 ? 0 : volume > 255 ? 255 : volume;

  if (s_context) {
    alListenerf(AL_GAIN, s_masterVolume / 255.0f);
  }
}

signed char FSOUND_3D_Listener_SetAttributes(
    const float *position,
    const float *velocity,
    float        forwardX,
    float        forwardY,
    float        forwardZ,
    float        topX,
    float        topY,
    float        topZ
) {
  if (position) {
    memcpy(s_listenerPos, position, sizeof(s_listenerPos));
  }
  if (velocity) {
    memcpy(s_listenerVel, velocity, sizeof(s_listenerVel));
  }

  s_listenerForward[0] = forwardX;
  s_listenerForward[1] = forwardY;
  s_listenerForward[2] = forwardZ;
  s_listenerTop[0] = topX;
  s_listenerTop[1] = topY;
  s_listenerTop[2] = topZ;

  if (s_context) {
    ApplyListener();
  }

  return 1;
}

void FSOUND_3D_Listener_GetAttributes(
    float *position,
    float *velocity,
    float *forwardX,
    float *forwardY,
    float *forwardZ,
    float *topX,
    float *topY,
    float *topZ
) {
  if (position) {
    memcpy(position, s_listenerPos, sizeof(s_listenerPos));
  }
  if (velocity) {
    memcpy(velocity, s_listenerVel, sizeof(s_listenerVel));
  }
  if (forwardX) {
    *forwardX = s_listenerForward[0];
  }
  if (forwardY) {
    *forwardY = s_listenerForward[1];
  }
  if (forwardZ) {
    *forwardZ = s_listenerForward[2];
  }
  if (topX) {
    *topX = s_listenerTop[0];
  }
  if (topY) {
    *topY = s_listenerTop[1];
  }
  if (topZ) {
    *topZ = s_listenerTop[2];
  }
}

FSOUND_STREAM *FSOUND_Stream_Open(const char *filename, unsigned int mode, int offset, int length) {
  unsigned char  header[16];
  int            formatTag = 0;
  int            channels = 0;
  int            rate = 0;
  int            blockAlign = 0;
  int            bits = 0;
  int            dataSize;
  unsigned int   file;
  FSOUND_STREAM *stream;

  if (!s_initialized) {
    s_error = FMOD_ERR_UNINITIALIZED;
    return 0;
  }

  file = s_open ? s_open(filename) : 0;
  if (!file) {
    s_error = FMOD_ERR_FILE_NOTFOUND;
    return 0;
  }

  if (s_read(header, 12, file) != 12 || memcmp(header, "RIFF", 4) || memcmp(header + 8, "WAVE", 4)) {
    s_close(file);
    s_error = FMOD_ERR_FILE_FORMAT;
    return 0;
  }

  for (;;) {
    if (s_read(header, 8, file) != 8) {
      s_close(file);
      s_error = FMOD_ERR_FILE_FORMAT;
      return 0;
    }

    int chunkSize = header[4] | header[5] << 8 | header[6] << 16 | header[7] << 24;

    if (!memcmp(header, "data", 4)) {
      dataSize = chunkSize;
      break;
    }

    if (!memcmp(header, "fmt ", 4) && chunkSize >= 16) {
      if (s_read(header, 16, file) != 16) {
        s_close(file);
        s_error = FMOD_ERR_FILE_FORMAT;
        return 0;
      }
      formatTag = header[0] | header[1] << 8;
      channels = header[2] | header[3] << 8;
      rate = header[4] | header[5] << 8 | header[6] << 16 | header[7] << 24;
      blockAlign = header[12] | header[13] << 8;
      bits = header[14] | header[15] << 8;
      chunkSize -= 16;
    }

    s_seek(file, chunkSize + (chunkSize & 1), 1);
  }

  if (formatTag != 1 || (channels != 1 && channels != 2) || (bits != 8 && bits != 16) || rate <= 0 || blockAlign != channels * bits / 8 ||
      dataSize < 0) {
    s_close(file);
    s_error = FMOD_ERR_FILE_FORMAT;
    return 0;
  }

  stream = (FSOUND_STREAM *)s_alloc(sizeof(FSOUND_STREAM));
  if (!stream) {
    s_close(file);
    s_error = FMOD_ERR_MEMORY;
    return 0;
  }

  memset(stream, 0, sizeof(*stream));
  stream->sample.minDistance = 1.0f;
  stream->sample.maxDistance = 1000000000.0f;
  stream->file = file;
  stream->mode = mode;
  stream->channel = -1;
  stream->loopCount = -1;
  stream->loopsLeft = -1;
  stream->format = channels == 1 ? (bits == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16) : (bits == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16);
  stream->rate = rate;
  stream->blockAlign = blockAlign;
  stream->dataStart = s_tell(file);
  stream->dataSize = dataSize - dataSize % blockAlign;
  stream->pcmSize = (rate / BUFFERS_PER_SECOND) * blockAlign;
  stream->pcm = (unsigned char *)s_alloc(stream->pcmSize);
  if (!stream->pcm) {
    s_free(stream);
    s_close(file);
    s_error = FMOD_ERR_MEMORY;
    return 0;
  }

  return stream;
}

signed char FSOUND_Stream_Close(FSOUND_STREAM *stream) {
  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  pthread_mutex_lock(&s_lock);
  if (stream->channel != -1) {
    ReleaseChannel(&s_channels[stream->channel & 0xFFFF]);
  }
  pthread_mutex_unlock(&s_lock);

  s_close(stream->file);
  s_free(stream->pcm);
  s_free(stream);
  return 1;
}

int FSOUND_Stream_PlayEx(int channel, FSOUND_STREAM *stream, void *dsp, signed char startpaused) {
  int handle = -1;

  if (!stream || channel != FSOUND_FREE) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return -1;
  }

  pthread_mutex_lock(&s_lock);

  if (stream->channel != -1 || stream->ended) {
    StopStream(stream);
  }

  for (int i = 0; i < s_numChannels; ++i) {
    Channel *target = &s_channels[i];

    if (target->stream) {
      continue;
    }

    target->stream = stream;
    target->volume = 255;
    target->paused = startpaused != 0;
    target->muted = false;
    handle = i | target->ref << 16;
    stream->channel = handle;

    bool is3d = !(stream->mode & FSOUND_2D);
    alSourcei(target->source, AL_SOURCE_RELATIVE, is3d ? AL_FALSE : AL_TRUE);
    alSource3f(target->source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(target->source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alSourcef(target->source, AL_REFERENCE_DISTANCE, stream->sample.minDistance);
    alSourcef(target->source, AL_MAX_DISTANCE, stream->sample.maxDistance);
    alSourcef(target->source, AL_ROLLOFF_FACTOR, is3d ? s_rolloffFactor : 0.0f);
    alSourcef(target->source, AL_PITCH, 1.0f);
    ApplyGain(target);
    QueueBuffers(target);
    break;
  }

  pthread_mutex_unlock(&s_lock);

  if (handle == -1) {
    s_error = FMOD_ERR_CHANNEL_ALLOC;
  }

  return handle;
}

signed char FSOUND_Stream_Stop(FSOUND_STREAM *stream) {
  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  pthread_mutex_lock(&s_lock);
  StopStream(stream);
  pthread_mutex_unlock(&s_lock);
  return 1;
}

signed char FSOUND_Stream_SetEndCallback(FSOUND_STREAM *stream, FSOUND_STREAMCALLBACK callback, intptr_t userdata) {
  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  pthread_mutex_lock(&s_lock);
  stream->endCallback = callback;
  stream->endUserData = userdata;
  pthread_mutex_unlock(&s_lock);
  return 1;
}

signed char FSOUND_Stream_SetLoopCount(FSOUND_STREAM *stream, int count) {
  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  pthread_mutex_lock(&s_lock);
  stream->loopCount = count;
  stream->loopsLeft = count;
  pthread_mutex_unlock(&s_lock);
  return 1;
}

int FSOUND_Stream_GetLengthMs(FSOUND_STREAM *stream) {
  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  return (int)((long long)(stream->dataSize / stream->blockAlign) * 1000 / stream->rate);
}

signed char FSOUND_Stream_SetTime(FSOUND_STREAM *stream, int milliseconds) {
  if (!stream || milliseconds < 0) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  int position = (int)((long long)milliseconds * stream->rate / 1000) * stream->blockAlign;
  if (position > stream->dataSize) {
    position = stream->dataSize;
  }

  pthread_mutex_lock(&s_lock);

  s_seek(stream->file, stream->dataStart + position, 0);
  stream->position = position;
  stream->ended = false;

  if (stream->channel != -1) {
    Channel *channel = &s_channels[stream->channel & 0xFFFF];
    alSourceRewind(channel->source);
    alSourcei(channel->source, AL_BUFFER, 0);
    QueueBuffers(channel);
  }

  pthread_mutex_unlock(&s_lock);
  return 1;
}

FSOUND_SAMPLE *FSOUND_Stream_GetSample(FSOUND_STREAM *stream) {
  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  return &stream->sample;
}

signed char FSOUND_Sample_SetMinMaxDistance(FSOUND_SAMPLE *sample, float minDistance, float maxDistance) {
  FSOUND_STREAM *stream = (FSOUND_STREAM *)sample;

  if (!sample || minDistance < 0.0f || maxDistance < 0.0f) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  pthread_mutex_lock(&s_lock);

  sample->minDistance = minDistance;
  sample->maxDistance = maxDistance;
  if (stream->channel != -1) {
    Channel *channel = &s_channels[stream->channel & 0xFFFF];
    alSourcef(channel->source, AL_REFERENCE_DISTANCE, minDistance);
    alSourcef(channel->source, AL_MAX_DISTANCE, maxDistance);
  }

  pthread_mutex_unlock(&s_lock);
  return 1;
}

signed char FSOUND_StopSound(int channel) {
  pthread_mutex_lock(&s_lock);

  if (channel == FSOUND_ALL) {
    for (int i = 0; i < s_numChannels; ++i) {
      if (s_channels[i].stream) {
        ReleaseChannel(&s_channels[i]);
      }
    }
    pthread_mutex_unlock(&s_lock);
    return 1;
  }

  Channel *stop = GetChannel(channel);
  if (stop) {
    ReleaseChannel(stop);
  }

  pthread_mutex_unlock(&s_lock);
  return stop != 0;
}

signed char FSOUND_IsPlaying(int channel) {
  pthread_mutex_lock(&s_lock);
  Channel *playing = GetChannel(channel);
  pthread_mutex_unlock(&s_lock);
  return playing != 0;
}

signed char FSOUND_SetPaused(int channel, signed char paused) {
  pthread_mutex_lock(&s_lock);

  Channel *pause = GetChannel(channel);
  if (pause) {
    ALint state = 0;
    pause->paused = paused != 0;
    alGetSourcei(pause->source, AL_SOURCE_STATE, &state);
    if (pause->paused) {
      alSourcePause(pause->source);
    } else if (state != AL_PLAYING) {
      alSourcePlay(pause->source);
    }
  }

  pthread_mutex_unlock(&s_lock);
  return pause != 0;
}

signed char FSOUND_SetMute(int channel, signed char mute) {
  pthread_mutex_lock(&s_lock);

  Channel *muted = GetChannel(channel);
  if (muted) {
    muted->muted = mute != 0;
    ApplyGain(muted);
  }

  pthread_mutex_unlock(&s_lock);
  return muted != 0;
}

signed char FSOUND_SetVolume(int channel, int volume) {
  pthread_mutex_lock(&s_lock);

  Channel *target = GetChannel(channel);
  if (target) {
    target->volume = volume < 0 ? 0 : volume > 255 ? 255 : volume;
    ApplyGain(target);
  }

  pthread_mutex_unlock(&s_lock);
  return target != 0;
}

signed char FSOUND_SetFrequency(int channel, int frequency) {
  pthread_mutex_lock(&s_lock);

  Channel *target = GetChannel(channel);
  if (target && frequency > 0) {
    alSourcef(target->source, AL_PITCH, (float)frequency / target->stream->rate);
  }

  pthread_mutex_unlock(&s_lock);
  return target != 0;
}

signed char FSOUND_3D_SetAttributes(int channel, const float *position, const float *velocity) {
  pthread_mutex_lock(&s_lock);

  Channel *target = GetChannel(channel);
  if (target && !(target->stream->mode & FSOUND_2D)) {
    if (position) {
      alSource3f(target->source, AL_POSITION, position[0], position[1], -position[2]);
    }
    if (velocity) {
      alSource3f(target->source, AL_VELOCITY, velocity[0], velocity[1], -velocity[2]);
    }
  }

  pthread_mutex_unlock(&s_lock);
  return target != 0;
}

}
