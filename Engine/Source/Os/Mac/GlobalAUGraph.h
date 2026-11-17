#pragma once

#include <AudioToolbox/AudioToolbox.h>

#include <ctype.h>
#include <unistd.h>

#ifdef ASSERTIONS_ENABLED
#define ASSERTVALUE(a, v)                                                                                                                  \
  if (!(a)) {                                                                                                                              \
    SErrDisplayErrorFmt(                                                                                                                   \
        STORM_ERROR_ASSERTION, __FILE__, __LINE__, FALSE, 1,                                                                               \
        isprint(((v) >> 24) & 0xFF) && isprint(((v) >> 16) & 0xFF) && isprint(((v) >> 8) & 0xFF) && isprint((v) & 0xFF)                   \
            ? "\"%s\", %s = %ld (0x%08X, '%c%c%c%c')"                                                                                      \
            : "\"%s\", %s = %ld (0x%08X)",                                                                                                 \
        #a, #v, (long)(v), (UInt32)(v), (int)(((v) >> 24) & 0xFF), (int)(((v) >> 16) & 0xFF), (int)(((v) >> 8) & 0xFF), (int)((v) & 0xFF) \
    );                                                                                                                                     \
  }
#else
#define ASSERTVALUE(a, v)
#endif

class Mixer {
 public:
  enum MixerDumpLevel {
    e_dumpQuiet = 0,
    e_dumpVerbose = 1
  };

  enum {
    MAX_INPUTS = 64
  };

  Mixer();
  ~Mixer();
  OSStatus Initialize(OSType mixerType);
  void     Destroy();
  OSStatus Connect(AUNode node, int sourceOutput);
  OSStatus Disconnect(AUNode node);
  OSStatus SetMixVolume(AUNode node, float volume);
  OSStatus Set3DDistance(AUNode node, float distance);
  OSStatus Set3DAzimuth(AUNode node, float azimuth);
  OSStatus Set3DElevation(AUNode node, float elevation);
  void     LabelNode(AUNode node, const char *name, const char *state);
  void     Dump(MixerDumpLevel level);

  OSType      m_mixerType;
  AUNode      m_mixerNode;
  AudioUnit   m_mixerUnit;
  AUNode      m_inputs[MAX_INPUTS];
  const char *m_nameLabels[MAX_INPUTS];
  const char *m_stateLabels[MAX_INPUTS];
  SCritSect   m_lock;
};

class GlobalAUGraph {
 public:
  void Initialize();
  void Destroy();

  static GlobalAUGraph &Get();
  static AUGraph        GetAUGraph() {
    return Get().m_graph;
  }

  AUGraph   m_graph;
  AUNode    m_outputNode;
  AudioUnit m_outputUnit;
  Mixer     m_mixer;
};

inline OSStatus AUGraphUpdateStable(AUGraph graph) {
  OSStatus err;
  while ((err = AUGraphUpdate(graph, 0)) == kAUGraphErr_CannotDoInCurrentContext) {
    usleep(500);
  }
  ASSERTVALUE(err == noErr, err);
  return err;
}
