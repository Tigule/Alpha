#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#define WINAPI
#endif

#include "../../miniaudio/miniaudio.h"

struct FSOUND_SAMPLE {
  float minDistance;
  float maxDistance;
};

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

struct FSOUND_STREAM {
  ma_data_source_base             base;
  ma_resource_manager_data_source source;
  FSOUND_SAMPLE                   sample;
  unsigned int                    mode;
  int                             channel;
  int                             loopCount;
  int                             loopsLeft;
  int                             channels;
  int                             rate;
  bool                            ended;
  FSOUND_STREAMCALLBACK           endCallback;
  intptr_t                        endUserData;
  FSOUND_STREAMCALLBACK           callback;
  intptr_t                        userData;
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
  FSOUND_OUTPUT_DSOUND = 2,
  FSOUND_OUTPUT_ALSA = 6,
  FSOUND_OUTPUT_MAC = 10,
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
  FSOUND_16BITS = 0x10,
  FSOUND_STEREO = 0x40,
  FSOUND_2D = 0x2000,
  FSOUND_FREE = -1,
  FSOUND_ALL = -3
};

enum {
  MAX_DRIVERS = 32
};

struct Channel {
  ma_sound       sound;
  FSOUND_STREAM *stream;
  int            ref;
  int            volume;
  int            frequency;
  bool           paused;
  bool           muted;
  float          position[3];
  float          velocity[3];
  float          mixPosition[3];
  float          mixVelocity[3];
  bool           placed;
  float          minDistance;
  float          maxDistance;
};

static void *WINAPI DefaultAlloc(unsigned int size) {
  return malloc(size);
}

static void *WINAPI DefaultRealloc(void *ptr, unsigned int size) {
  return realloc(ptr, size);
}

static void WINAPI DefaultFree(void *ptr) {
  free(ptr);
}

static std::recursive_mutex   s_lock;
static bool                   s_initialized;
static bool                   s_haveContext;
static ma_context             s_context;
static ma_resource_manager    s_resourceManager;
static ma_engine              s_engine;
static ma_vfs_callbacks       s_vfs;
static Channel               *s_channels;
static int                    s_numChannels;
static int                    s_error;
static int                    s_output = -1;
static int                    s_driver;
static int                    s_mixer = FSOUND_MIXER_QUALITY_AUTODETECT;
static int                    s_mixRate = 44100;
static int                    s_masterVolume = 255;
static float                  s_distanceFactor = 1.0f;
static float                  s_dopplerFactor = 1.0f;
static float                  s_rolloffFactor = 1.0f;
static float                  s_listenerPos[3];
static float                  s_listenerVel[3];
static float                  s_listenerForward[3] = {0.0f, 0.0f, 1.0f};
static float                  s_listenerTop[3] = {0.0f, 1.0f, 0.0f};
static float                  s_mixListenerPos[3];
static float                  s_mixListenerVel[3];
static float                  s_mixListenerForward[3] = {0.0f, 0.0f, 1.0f};
static float                  s_mixListenerTop[3] = {0.0f, 1.0f, 0.0f};
static FSOUND_ALLOCCALLBACK   s_alloc = DefaultAlloc;
static FSOUND_REALLOCCALLBACK s_realloc = DefaultRealloc;
static FSOUND_FREECALLBACK    s_free = DefaultFree;
static FSOUND_OPENCALLBACK    s_open;
static FSOUND_CLOSECALLBACK   s_close;
static FSOUND_READCALLBACK    s_read;
static FSOUND_SEEKCALLBACK    s_seek;
static FSOUND_TELLCALLBACK    s_tell;
static int                    s_numDrivers = -1;
static const char            *s_driverNames[MAX_DRIVERS];
static ma_device_id           s_driverIds[MAX_DRIVERS];
static char                   s_driverNameData[4096];
static int                    s_driverNameUsed;

static void *AllocCallback(size_t size, void *) {
  return s_alloc((unsigned int)size);
}

static void *ReallocCallback(void *ptr, size_t size, void *) {
  return s_realloc(ptr, (unsigned int)size);
}

static void FreeCallback(void *ptr, void *) {
  if (ptr) {
    s_free(ptr);
  }
}

static ma_result VfsOpen(ma_vfs *, const char *path, ma_uint32, ma_vfs_file *file) {
  unsigned int handle = s_open ? s_open(path) : 0;

  *file = (ma_vfs_file)(uintptr_t)handle;
  return handle ? MA_SUCCESS : MA_DOES_NOT_EXIST;
}

static ma_result VfsClose(ma_vfs *, ma_vfs_file file) {
  s_close((unsigned int)(uintptr_t)file);
  return MA_SUCCESS;
}

static ma_result VfsRead(ma_vfs *, ma_vfs_file file, void *buffer, size_t size, size_t *bytesRead) {
  int count = s_read(buffer, (int)size, (unsigned int)(uintptr_t)file);

  *bytesRead = count > 0 ? count : 0;
  return *bytesRead || !size ? MA_SUCCESS : MA_AT_END;
}

static ma_result VfsSeek(ma_vfs *, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin) {
  return s_seek((unsigned int)(uintptr_t)file, (int)offset, (signed char)origin) >= 0 ? MA_SUCCESS : MA_ERROR;
}

static ma_result VfsTell(ma_vfs *, ma_vfs_file file, ma_int64 *cursor) {
  *cursor = s_tell((unsigned int)(uintptr_t)file);
  return MA_SUCCESS;
}

static ma_result VfsInfo(ma_vfs *, ma_vfs_file file, ma_file_info *info) {
  unsigned int handle = (unsigned int)(uintptr_t)file;
  int          cursor = s_tell(handle);

  s_seek(handle, 0, ma_seek_origin_end);
  info->sizeInBytes = s_tell(handle);
  s_seek(handle, cursor, ma_seek_origin_start);
  return MA_SUCCESS;
}

static ma_result StreamRead(ma_data_source *source, void *output, ma_uint64 frameCount, ma_uint64 *framesRead) {
  FSOUND_STREAM *stream = (FSOUND_STREAM *)source;
  ma_uint64      total = 0;
  ma_result      result = MA_SUCCESS;
  bool           rewound = false;

  if (stream->callback) {
    short scratch[1024];

    while (total < frameCount) {
      ma_uint64 count = frameCount - total;
      short    *target = output ? (short *)output + total * stream->channels : scratch;

      if (!output && count > sizeof(scratch) / sizeof(short) / stream->channels) {
        count = sizeof(scratch) / sizeof(short) / stream->channels;
      }
      stream->callback(stream, target, (int)(count * stream->channels * sizeof(short)), stream->userData);
      total += count;
    }

    *framesRead = total;
    return MA_SUCCESS;
  }

  while (total < frameCount) {
    ma_uint64 count = 0;
    void     *target = output ? (char *)output + total * stream->channels * sizeof(short) : 0;

    result = ma_resource_manager_data_source_read_pcm_frames(&stream->source, target, frameCount - total, &count);
    if (count) {
      total += count;
      rewound = false;
      continue;
    }

    if (result != MA_AT_END || !(stream->mode & FSOUND_LOOP_NORMAL) || !stream->loopsLeft || rewound) {
      break;
    }
    if (stream->loopsLeft > 0) {
      --stream->loopsLeft;
    }
    ma_resource_manager_data_source_seek_to_pcm_frame(&stream->source, 0);
    rewound = true;
  }

  *framesRead = total;
  if (total) {
    return MA_SUCCESS;
  }

  return result == MA_SUCCESS ? MA_AT_END : result;
}

static ma_result StreamSeek(ma_data_source *source, ma_uint64 frame) {
  if (((FSOUND_STREAM *)source)->callback) {
    return MA_NOT_IMPLEMENTED;
  }
  return ma_resource_manager_data_source_seek_to_pcm_frame(&((FSOUND_STREAM *)source)->source, frame);
}

static ma_result StreamGetDataFormat(
    ma_data_source *source,
    ma_format      *format,
    ma_uint32      *channels,
    ma_uint32      *sampleRate,
    ma_channel     *channelMap,
    size_t          channelMapCap
) {
  FSOUND_STREAM *stream = (FSOUND_STREAM *)source;

  if (stream->callback) {
    *format = ma_format_s16;
    *channels = stream->channels;
    *sampleRate = stream->rate;
    ma_channel_map_init_standard(ma_standard_channel_map_default, channelMap, channelMapCap, stream->channels);
    return MA_SUCCESS;
  }
  return ma_resource_manager_data_source_get_data_format(&((FSOUND_STREAM *)source)->source, format, channels, sampleRate, channelMap, channelMapCap);
}

static ma_result StreamGetCursor(ma_data_source *source, ma_uint64 *cursor) {
  if (((FSOUND_STREAM *)source)->callback) {
    *cursor = 0;
    return MA_NOT_IMPLEMENTED;
  }
  return ma_resource_manager_data_source_get_cursor_in_pcm_frames(&((FSOUND_STREAM *)source)->source, cursor);
}

static ma_result StreamGetLength(ma_data_source *source, ma_uint64 *length) {
  if (((FSOUND_STREAM *)source)->callback) {
    *length = 0;
    return MA_NOT_IMPLEMENTED;
  }
  return ma_resource_manager_data_source_get_length_in_pcm_frames(&((FSOUND_STREAM *)source)->source, length);
}

static ma_data_source_vtable s_streamVtable = {StreamRead, StreamSeek, StreamGetDataFormat, StreamGetCursor, StreamGetLength, 0, 0};

static void AddDriver(const char *name, const ma_device_id *id) {
  int length = strlen(name) + 1;

  if (s_numDrivers >= MAX_DRIVERS || s_driverNameUsed + length > (int)sizeof(s_driverNameData)) {
    return;
  }

  memcpy(s_driverNameData + s_driverNameUsed, name, length);
  if (id) {
    s_driverIds[s_numDrivers] = *id;
  }
  s_driverNames[s_numDrivers++] = s_driverNameData + s_driverNameUsed;
  s_driverNameUsed += length;
}

static void EnumerateDrivers() {
  ma_device_info *devices;
  ma_uint32       count;

  if (s_numDrivers >= 0) {
    return;
  }

  s_numDrivers = 0;
  s_driverNameUsed = 0;

  if (!s_haveContext) {
    s_haveContext = ma_context_init(0, 0, 0, &s_context) == MA_SUCCESS;
  }
  if (!s_haveContext || ma_context_get_devices(&s_context, &devices, &count, 0, 0) != MA_SUCCESS) {
    return;
  }

  for (ma_uint32 i = 0; i < count; ++i) {
    if (devices[i].isDefault) {
      AddDriver(devices[i].name, 0);
    }
  }
  if (!s_numDrivers) {
    AddDriver("Default", 0);
  }

  for (ma_uint32 i = 0; i < count; ++i) {
    if (!devices[i].isDefault) {
      AddDriver(devices[i].name, &devices[i].id);
    }
  }
}

static Channel *GetChannel(int handle) {
  int index = handle & 0xFFFF;

  if (handle < 0 || index >= s_numChannels || !s_channels[index].stream || s_channels[index].ref != (handle >> 16)) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  return &s_channels[index];
}

static void UpdateChannel(Channel *channel) {
  FSOUND_STREAM *stream = channel->stream;
  float          gain = channel->muted ? 0.0f : channel->volume / 255.0f;
  float          pitch = (float)channel->frequency / stream->rate;

  if (stream->channels != 1) {
    ma_sound_set_volume(&channel->sound, gain);
    ma_sound_set_pitch(&channel->sound, pitch);
    return;
  }

  float left = 0.70710678f;
  float right = 0.70710678f;

  if (!(stream->mode & FSOUND_2D)) {
    float rel[3] = {
        channel->mixPosition[0] - s_mixListenerPos[0], channel->mixPosition[1] - s_mixListenerPos[1], channel->mixPosition[2] - s_mixListenerPos[2]
    };
    float side[3] = {
        s_mixListenerTop[1] * s_mixListenerForward[2] - s_mixListenerTop[2] * s_mixListenerForward[1],
        s_mixListenerTop[2] * s_mixListenerForward[0] - s_mixListenerTop[0] * s_mixListenerForward[2],
        s_mixListenerTop[0] * s_mixListenerForward[1] - s_mixListenerTop[1] * s_mixListenerForward[0]
    };
    float x = rel[0] * side[0] + rel[1] * side[1] + rel[2] * side[2];
    float z = rel[0] * s_mixListenerForward[0] + rel[1] * s_mixListenerForward[1] + rel[2] * s_mixListenerForward[2];
    float horizontal = sqrtf(x * x + z * z);
    float distance = sqrtf(rel[0] * rel[0] + rel[1] * rel[1] + rel[2] * rel[2]);
    float clamped = distance < channel->minDistance ? channel->minDistance : distance > channel->maxDistance ? channel->maxDistance : distance;
    float pan = 0.0f;

    if (horizontal > 0.0f) {
      pan = x / (horizontal + fabsf(z));
      if (horizontal < 2.5f) {
        pan *= horizontal / 2.5f;
      }
    }

    gain = powf(gain, 1.660964f) * 0.42f;
    if (clamped > channel->minDistance) {
      gain *= channel->minDistance / (channel->minDistance + s_rolloffFactor * (clamped - channel->minDistance));
    }
    left = sqrtf(1.0f - pan);
    right = sqrtf(1.0f + pan);

    if (distance > 0.0f && s_dopplerFactor > 0.0f) {
      float speed = 343.3f * s_distanceFactor;
      float limit = speed / s_dopplerFactor;
      float listenerSpeed = -(rel[0] * s_mixListenerVel[0] + rel[1] * s_mixListenerVel[1] + rel[2] * s_mixListenerVel[2]) / distance;
      float sourceSpeed = -(rel[0] * channel->mixVelocity[0] + rel[1] * channel->mixVelocity[1] + rel[2] * channel->mixVelocity[2]) / distance;
      listenerSpeed = listenerSpeed < limit ? listenerSpeed : limit;
      sourceSpeed = sourceSpeed < limit ? sourceSpeed : limit;
      if (speed - s_dopplerFactor * sourceSpeed > 0.0f) {
        pitch *= (speed - s_dopplerFactor * listenerSpeed) / (speed - s_dopplerFactor * sourceSpeed);
      }
    }
  }

  if (left >= right) {
    ma_sound_set_volume(&channel->sound, gain * left);
    ma_sound_set_pan(&channel->sound, left > 0.0f ? right / left - 1.0f : 0.0f);
  } else {
    ma_sound_set_volume(&channel->sound, gain * right);
    ma_sound_set_pan(&channel->sound, 1.0f - left / right);
  }
  ma_sound_set_pitch(&channel->sound, pitch);
}

static void ReleaseChannel(Channel *channel) {
  ma_sound_uninit(&channel->sound);
  channel->stream->channel = -1;
  channel->stream = 0;
  channel->ref = channel->ref % 0x7FFF + 1;
}

static void ProcessEndedChannels() {
  for (int i = 0; i < s_numChannels; ++i) {
    Channel       *channel = &s_channels[i];
    FSOUND_STREAM *stream = channel->stream;

    if (!stream || !ma_sound_at_end(&channel->sound)) {
      continue;
    }

    ReleaseChannel(channel);
    stream->ended = true;
    if (stream->endCallback) {
      stream->endCallback(stream, 0, 0, stream->endUserData);
    }
  }
}

static void StopStream(FSOUND_STREAM *stream) {
  if (stream->channel != -1) {
    ReleaseChannel(&s_channels[stream->channel & 0xFFFF]);
  }

  if (!stream->callback) {
    ma_resource_manager_data_source_seek_to_pcm_frame(&stream->source, 0);
  }
  stream->loopsLeft = stream->loopCount;
  stream->ended = false;
}

extern "C" {

signed char WINAPI FSOUND_SetOutput(int outputtype) {
  if (s_initialized) {
    s_error = FMOD_ERR_BUSY;
    return 0;
  }

  if (outputtype < -1 || outputtype > FSOUND_OUTPUT_NOSOUND_NONREALTIME) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  s_output = outputtype;
  return 1;
}

signed char WINAPI FSOUND_SetHWND(unsigned long window) {
  return 1;
}

int WINAPI FSOUND_GetOutput() {
  if (s_output != -1) {
    return s_output;
  }

#if defined(_WIN32)
  return FSOUND_OUTPUT_DSOUND;
#elif defined(__APPLE__)
  return FSOUND_OUTPUT_MAC;
#else
  return FSOUND_OUTPUT_ALSA;
#endif
}

int WINAPI FSOUND_GetNumDrivers() {
  EnumerateDrivers();
  return s_numDrivers;
}

const char *WINAPI FSOUND_GetDriverName(int driver) {
  EnumerateDrivers();

  if (driver < 0 || driver >= s_numDrivers) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return "";
  }

  return s_driverNames[driver];
}

signed char WINAPI FSOUND_SetDriver(int driver) {
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

int WINAPI FSOUND_GetDriver() {
  return s_driver;
}

signed char WINAPI FSOUND_GetDriverCaps(int driver, unsigned int *caps) {
  *caps = 0;
  return 1;
}

signed char WINAPI FSOUND_SetMixer(int mixer) {
  if (mixer < 0 || mixer >= FSOUND_MIXER_MAX) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  s_mixer = mixer;
  return 1;
}

int WINAPI FSOUND_GetMixer() {
  if (s_mixer == FSOUND_MIXER_AUTODETECT) {
    return FSOUND_MIXER_BLENDMODE;
  }

  if (s_mixer == FSOUND_MIXER_QUALITY_AUTODETECT) {
    return FSOUND_MIXER_QUALITY_FPU;
  }

  return s_mixer;
}

signed char WINAPI FSOUND_SetBufferSize(int milliseconds) {
  if (s_initialized) {
    s_error = FMOD_ERR_BUSY;
    return 0;
  }

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
  if (s_initialized) {
    s_error = FMOD_ERR_BUSY;
    return 0;
  }

  if (pool || !allocCallback || !reallocCallback || !freeCallback) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  s_alloc = allocCallback;
  s_realloc = reallocCallback;
  s_free = freeCallback;
  return 1;
}

void WINAPI FSOUND_File_SetCallbacks(
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

void WINAPI FSOUND_3D_SetDistanceFactor(float factor) {
  std::lock_guard<std::recursive_mutex> guard(s_lock);
  s_distanceFactor = factor;
}

void WINAPI FSOUND_3D_SetDopplerFactor(float factor) {
  std::lock_guard<std::recursive_mutex> guard(s_lock);
  s_dopplerFactor = factor;
}

void WINAPI FSOUND_3D_SetRolloffFactor(float factor) {
  std::lock_guard<std::recursive_mutex> guard(s_lock);

  s_rolloffFactor = factor;
  for (int i = 0; i < s_numChannels; ++i) {
    if (s_channels[i].stream) {
      UpdateChannel(&s_channels[i]);
    }
  }
}

void WINAPI FSOUND_Close();

signed char WINAPI FSOUND_Init(int mixrate, int maxsoftwarechannels, unsigned int flags) {
  if (s_initialized) {
    s_error = FMOD_ERR_BUSY;
    return 0;
  }

  if (mixrate < 4000 || mixrate > 65535 || maxsoftwarechannels < 0 || maxsoftwarechannels > 1024) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  ma_allocation_callbacks callbacks = {0, AllocCallback, ReallocCallback, FreeCallback};

  s_vfs.onOpen = VfsOpen;
  s_vfs.onClose = VfsClose;
  s_vfs.onRead = VfsRead;
  s_vfs.onSeek = VfsSeek;
  s_vfs.onTell = VfsTell;
  s_vfs.onInfo = VfsInfo;

  ma_resource_manager_config managerConfig = ma_resource_manager_config_init();
  managerConfig.allocationCallbacks = callbacks;
  managerConfig.pVFS = &s_vfs;
  managerConfig.decodedFormat = ma_format_s16;
  if (ma_resource_manager_init(&managerConfig, &s_resourceManager) != MA_SUCCESS) {
    s_error = FMOD_ERR_INIT;
    return 0;
  }

  EnumerateDrivers();

  ma_engine_config engineConfig = ma_engine_config_init();
  engineConfig.pContext = s_haveContext ? &s_context : 0;
  engineConfig.pPlaybackDeviceID = s_driver > 0 && s_driver < s_numDrivers ? &s_driverIds[s_driver] : 0;
  engineConfig.pResourceManager = &s_resourceManager;
  engineConfig.allocationCallbacks = callbacks;
  engineConfig.channels = 2;
  engineConfig.sampleRate = mixrate;
  engineConfig.listenerCount = 1;
  engineConfig.noDevice = s_output == FSOUND_OUTPUT_NOSOUND || s_output == FSOUND_OUTPUT_NOSOUND_NONREALTIME;
  if (ma_engine_init(&engineConfig, &s_engine) != MA_SUCCESS) {
    ma_resource_manager_uninit(&s_resourceManager);
    s_error = FMOD_ERR_INIT;
    return 0;
  }

  s_mixRate = mixrate;
  s_initialized = true;

  s_channels = (Channel *)s_alloc(maxsoftwarechannels * sizeof(Channel));
  if (!s_channels) {
    FSOUND_Close();
    s_error = FMOD_ERR_MEMORY;
    return 0;
  }

  memset(s_channels, 0, maxsoftwarechannels * sizeof(Channel));
  for (s_numChannels = 0; s_numChannels < maxsoftwarechannels; ++s_numChannels) {
    s_channels[s_numChannels].ref = 1;
  }

  ma_engine_set_volume(&s_engine, s_masterVolume / 255.0f);
  return 1;
}

void WINAPI FSOUND_Close() {
  if (!s_initialized) {
    return;
  }

  std::lock_guard<std::recursive_mutex> guard(s_lock);

  for (int i = 0; i < s_numChannels; ++i) {
    if (s_channels[i].stream) {
      ReleaseChannel(&s_channels[i]);
    }
  }

  if (s_channels) {
    s_free(s_channels);
    s_channels = 0;
  }
  s_numChannels = 0;

  ma_engine_uninit(&s_engine);
  ma_resource_manager_uninit(&s_resourceManager);

  if (s_haveContext) {
    ma_context_uninit(&s_context);
    s_haveContext = false;
    s_numDrivers = -1;
  }

  s_initialized = false;
}

void WINAPI FSOUND_Update() {
  std::lock_guard<std::recursive_mutex> guard(s_lock);
  ProcessEndedChannels();

  memcpy(s_mixListenerPos, s_listenerPos, sizeof(s_mixListenerPos));
  memcpy(s_mixListenerVel, s_listenerVel, sizeof(s_mixListenerVel));
  memcpy(s_mixListenerForward, s_listenerForward, sizeof(s_mixListenerForward));
  memcpy(s_mixListenerTop, s_listenerTop, sizeof(s_mixListenerTop));

  for (int i = 0; i < s_numChannels; ++i) {
    Channel *channel = &s_channels[i];

    if (channel->stream && !(channel->stream->mode & FSOUND_2D)) {
      memcpy(channel->mixPosition, channel->position, sizeof(channel->mixPosition));
      memcpy(channel->mixVelocity, channel->velocity, sizeof(channel->mixVelocity));
      channel->placed = true;
      UpdateChannel(channel);
    }
  }
}

int WINAPI FSOUND_GetError() {
  return s_error;
}

int WINAPI FSOUND_GetMaxChannels() {
  return s_numChannels;
}

int WINAPI FSOUND_GetNumHardwareChannels() {
  return 0;
}

int WINAPI FSOUND_GetOutputRate() {
  return s_initialized ? (int)ma_engine_get_sample_rate(&s_engine) : s_mixRate;
}

float WINAPI FSOUND_GetCPUUsage() {
  return 0.0f;
}

void WINAPI FSOUND_SetSFXMasterVolume(int volume) {
  s_masterVolume = volume < 0 ? 0 : volume > 255 ? 255 : volume;

  if (s_initialized) {
    ma_engine_set_volume(&s_engine, s_masterVolume / 255.0f);
  }
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
  std::lock_guard<std::recursive_mutex> guard(s_lock);

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

  return 1;
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
  std::lock_guard<std::recursive_mutex> guard(s_lock);

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

FSOUND_STREAM *WINAPI FSOUND_Stream_Open(const char *filename, unsigned int mode, int offset, int length) {
  ma_format      format;
  ma_uint32      channels;
  ma_uint32      rate;
  ma_result      result;
  FSOUND_STREAM *stream;

  if (!s_initialized) {
    s_error = FMOD_ERR_UNINITIALIZED;
    return 0;
  }

  stream = (FSOUND_STREAM *)s_alloc(sizeof(FSOUND_STREAM));
  if (!stream) {
    s_error = FMOD_ERR_MEMORY;
    return 0;
  }

  memset(stream, 0, sizeof(*stream));

  result = ma_resource_manager_data_source_init(
      &s_resourceManager,
      filename,
      MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_DECODE | MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_ASYNC | MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_WAIT_INIT,
      0,
      &stream->source
  );
  if (result != MA_SUCCESS) {
    s_free(stream);
    s_error = result == MA_DOES_NOT_EXIST ? FMOD_ERR_FILE_NOTFOUND : FMOD_ERR_FILE_FORMAT;
    return 0;
  }

  if (ma_resource_manager_data_source_get_data_format(&stream->source, &format, &channels, &rate, 0, 0) != MA_SUCCESS ||
      (channels != 1 && channels != 2) || !rate) {
    ma_resource_manager_data_source_uninit(&stream->source);
    s_free(stream);
    s_error = FMOD_ERR_FILE_FORMAT;
    return 0;
  }

  ma_data_source_config config = ma_data_source_config_init();
  config.vtable = &s_streamVtable;
  ma_data_source_init(&config, &stream->base);

  stream->sample.minDistance = 1.0f;
  stream->sample.maxDistance = 1000000000.0f;
  stream->mode = mode;
  stream->channel = -1;
  stream->loopCount = -1;
  stream->loopsLeft = -1;
  stream->channels = channels;
  stream->rate = rate;
  return stream;
}

FSOUND_STREAM *WINAPI FSOUND_Stream_Create(FSOUND_STREAMCALLBACK callback, int lenbytes, unsigned int mode, int samplerate, intptr_t userdata) {
  if (!s_initialized) {
    s_error = FMOD_ERR_UNINITIALIZED;
    return 0;
  }

  if (!callback || lenbytes <= 0 || samplerate <= 0 || !(mode & FSOUND_16BITS)) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  FSOUND_STREAM *stream = (FSOUND_STREAM *)s_alloc(sizeof(FSOUND_STREAM));
  if (!stream) {
    s_error = FMOD_ERR_MEMORY;
    return 0;
  }

  memset(stream, 0, sizeof(*stream));

  ma_data_source_config config = ma_data_source_config_init();
  config.vtable = &s_streamVtable;
  ma_data_source_init(&config, &stream->base);

  stream->sample.minDistance = 1.0f;
  stream->sample.maxDistance = 1000000000.0f;
  stream->mode = mode;
  stream->channel = -1;
  stream->loopCount = -1;
  stream->loopsLeft = -1;
  stream->channels = mode & FSOUND_STEREO ? 2 : 1;
  stream->rate = samplerate;
  stream->callback = callback;
  stream->userData = userdata;
  return stream;
}

signed char WINAPI FSOUND_Stream_Close(FSOUND_STREAM *stream) {
  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  std::lock_guard<std::recursive_mutex> guard(s_lock);

  if (stream->channel != -1) {
    ReleaseChannel(&s_channels[stream->channel & 0xFFFF]);
  }

  ma_data_source_uninit(&stream->base);
  if (s_initialized && !stream->callback) {
    ma_resource_manager_data_source_uninit(&stream->source);
  }
  s_free(stream);
  return 1;
}

int WINAPI FSOUND_Stream_PlayEx(int channel, FSOUND_STREAM *stream, void *dsp, signed char startpaused) {
  int handle = -1;

  if (!stream || channel != FSOUND_FREE) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return -1;
  }

  std::lock_guard<std::recursive_mutex> guard(s_lock);

  ProcessEndedChannels();

  if (stream->channel != -1 || stream->ended) {
    StopStream(stream);
  }

  for (int i = 0; i < s_numChannels; ++i) {
    Channel *target = &s_channels[i];

    if (target->stream) {
      continue;
    }

    if (ma_sound_init_from_data_source(&s_engine, &stream->base, MA_SOUND_FLAG_NO_SPATIALIZATION, 0, &target->sound) != MA_SUCCESS) {
      break;
    }

    target->stream = stream;
    target->volume = 255;
    target->paused = startpaused != 0;
    target->muted = false;
    target->frequency = stream->rate;
    memset(target->position, 0, sizeof(target->position));
    memset(target->velocity, 0, sizeof(target->velocity));
    memset(target->mixPosition, 0, sizeof(target->mixPosition));
    memset(target->mixVelocity, 0, sizeof(target->mixVelocity));
    target->placed = false;
    target->minDistance = stream->sample.minDistance;
    target->maxDistance = stream->sample.maxDistance;
    handle = i | target->ref << 16;
    stream->channel = handle;

    UpdateChannel(target);
    if (!target->paused) {
      ma_sound_start(&target->sound);
    }
    break;
  }

  if (handle == -1) {
    s_error = FMOD_ERR_CHANNEL_ALLOC;
  }

  return handle;
}

signed char WINAPI FSOUND_Stream_Stop(FSOUND_STREAM *stream) {
  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  std::lock_guard<std::recursive_mutex> guard(s_lock);
  StopStream(stream);
  return 1;
}

signed char WINAPI FSOUND_Stream_SetEndCallback(FSOUND_STREAM *stream, FSOUND_STREAMCALLBACK callback, intptr_t userdata) {
  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  std::lock_guard<std::recursive_mutex> guard(s_lock);
  stream->endCallback = callback;
  stream->endUserData = userdata;
  return 1;
}

signed char WINAPI FSOUND_Stream_SetLoopCount(FSOUND_STREAM *stream, int count) {
  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  std::lock_guard<std::recursive_mutex> guard(s_lock);
  stream->loopCount = count;
  stream->loopsLeft = count;
  return 1;
}

int WINAPI FSOUND_Stream_GetLengthMs(FSOUND_STREAM *stream) {
  ma_uint64 frames = 0;

  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  if (!stream->callback) {
    ma_resource_manager_data_source_get_length_in_pcm_frames(&stream->source, &frames);
  }
  return (int)(frames * 1000 / stream->rate);
}

signed char WINAPI FSOUND_Stream_SetTime(FSOUND_STREAM *stream, int milliseconds) {
  if (!stream || milliseconds < 0) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  std::lock_guard<std::recursive_mutex> guard(s_lock);

  ma_uint64 frame = (ma_uint64)milliseconds * stream->rate / 1000;

  stream->ended = false;
  if (stream->callback) {
    return 1;
  }
  if (stream->channel != -1) {
    ma_sound_seek_to_pcm_frame(&s_channels[stream->channel & 0xFFFF].sound, frame);
  } else {
    ma_resource_manager_data_source_seek_to_pcm_frame(&stream->source, frame);
  }

  return 1;
}

FSOUND_SAMPLE *WINAPI FSOUND_Stream_GetSample(FSOUND_STREAM *stream) {
  if (!stream) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  return &stream->sample;
}

signed char WINAPI FSOUND_Sample_SetMinMaxDistance(FSOUND_SAMPLE *sample, float minDistance, float maxDistance) {
  if (!sample || minDistance < 0.0f || maxDistance < 0.0f) {
    s_error = FMOD_ERR_INVALID_PARAM;
    return 0;
  }

  sample->minDistance = minDistance;
  sample->maxDistance = maxDistance;
  return 1;
}

signed char WINAPI FSOUND_StopSound(int channel) {
  std::lock_guard<std::recursive_mutex> guard(s_lock);

  if (channel == FSOUND_ALL) {
    for (int i = 0; i < s_numChannels; ++i) {
      if (s_channels[i].stream) {
        ReleaseChannel(&s_channels[i]);
      }
    }
    return 1;
  }

  Channel *stop = GetChannel(channel);
  if (stop) {
    ReleaseChannel(stop);
  }

  return stop != 0;
}

signed char WINAPI FSOUND_IsPlaying(int channel) {
  std::lock_guard<std::recursive_mutex> guard(s_lock);
  Channel                              *playing = GetChannel(channel);
  return playing && !ma_sound_at_end(&playing->sound);
}

signed char WINAPI FSOUND_SetPaused(int channel, signed char paused) {
  std::lock_guard<std::recursive_mutex> guard(s_lock);

  Channel *pause = GetChannel(channel);
  if (pause && !ma_sound_at_end(&pause->sound)) {
    pause->paused = paused != 0;
    if (pause->paused) {
      ma_sound_stop(&pause->sound);
    } else {
      ma_sound_start(&pause->sound);
    }
  }

  return pause != 0;
}

signed char WINAPI FSOUND_SetMute(int channel, signed char mute) {
  std::lock_guard<std::recursive_mutex> guard(s_lock);

  Channel *muted = GetChannel(channel);
  if (muted) {
    muted->muted = mute != 0;
    UpdateChannel(muted);
  }

  return muted != 0;
}

signed char WINAPI FSOUND_SetVolume(int channel, int volume) {
  std::lock_guard<std::recursive_mutex> guard(s_lock);

  Channel *target = GetChannel(channel);
  if (target) {
    target->volume = volume < 0 ? 0 : volume > 255 ? 255 : volume;
    UpdateChannel(target);
  }

  return target != 0;
}

signed char WINAPI FSOUND_SetPan(int channel, int pan) {
  std::lock_guard<std::recursive_mutex> guard(s_lock);
  return GetChannel(channel) != 0;
}

signed char WINAPI FSOUND_SetFrequency(int channel, int frequency) {
  std::lock_guard<std::recursive_mutex> guard(s_lock);

  Channel *target = GetChannel(channel);
  if (target && frequency > 0) {
    target->frequency = frequency;
    UpdateChannel(target);
  }

  return target != 0;
}

signed char WINAPI FSOUND_3D_SetAttributes(int channel, const float *position, const float *velocity) {
  std::lock_guard<std::recursive_mutex> guard(s_lock);

  Channel *target = GetChannel(channel);
  if (target && !(target->stream->mode & FSOUND_2D)) {
    if (position) {
      memcpy(target->position, position, sizeof(target->position));
    }
    if (velocity) {
      memcpy(target->velocity, velocity, sizeof(target->velocity));
    }
    if (!target->placed) {
      memcpy(target->mixPosition, target->position, sizeof(target->mixPosition));
      memcpy(target->mixVelocity, target->velocity, sizeof(target->mixVelocity));
      target->placed = true;
      UpdateChannel(target);
    }
  }

  return target != 0;
}
}
