#include <Base/Base.h>

#include <storm.h>

#include "Os/Mac/GlobalAUGraph.h"

#include <math.h>
#include <stdio.h>

static GlobalAUGraph s_globalAU;

GlobalAUGraph &GlobalAUGraph::Get() {
  return s_globalAU;
}

void GlobalAUGraph::Initialize() {
  OSStatus                  err;
  AudioComponentDescription desc;

  err = NewAUGraph(&m_graph);
  ASSERTVALUE(err == noErr, err);

  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_DefaultOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;
  err = AUGraphAddNode(m_graph, &desc, &m_outputNode);
  ASSERTVALUE(err == noErr, err);

  err = AUGraphOpen(m_graph);
  ASSERTVALUE(err == noErr, err);

  err = AUGraphInitialize(m_graph);
  ASSERTVALUE(err == noErr, err);

  err = AUGraphNodeInfo(m_graph, m_outputNode, 0, &m_outputUnit);
  ASSERTVALUE(err == noErr, err);

  err = AUGraphStart(m_graph);
  ASSERTVALUE(err == noErr, err);

  err = m_mixer.Initialize(kAudioUnitSubType_3DMixer);
  ASSERTVALUE(err == noErr, err);

  err = AUGraphConnectNodeInput(m_graph, m_mixer.m_mixerNode, 0, m_outputNode, 0);
  ASSERTVALUE(err == noErr, err);

  err = AUGraphUpdateStable(m_graph);
  ASSERTVALUE(err == noErr, err);
}

void GlobalAUGraph::Destroy() {
  OSStatus err;

  err = AUGraphStop(m_graph);
  ASSERTVALUE(err == noErr, err);

  err = AUGraphUninitialize(m_graph);
  ASSERTVALUE(err == noErr, err);

  err = AUGraphClose(m_graph);
  ASSERTVALUE(err == noErr, err);

  m_mixer.Destroy();

  err = AUGraphRemoveNode(m_graph, m_outputNode);
  ASSERTVALUE(err == noErr, err);

  err = DisposeAUGraph(m_graph);
  ASSERTVALUE(err == noErr, err);

  m_graph = 0;
}

Mixer::Mixer()
    : m_mixerType(0),
      m_mixerNode(0),
      m_mixerUnit(0) {
  for (int i = 0; i < MAX_INPUTS; ++i) {
    m_inputs[i] = 0;
    m_nameLabels[i] = 0;
    m_stateLabels[i] = 0;
  }
}

Mixer::~Mixer() {
  Destroy();
}

void Mixer::Destroy() {
  OSStatus err;

  if (!m_mixerNode || !GlobalAUGraph::GetAUGraph()) {
    return;
  }

  for (int i = 0; i < MAX_INPUTS; ++i) {
    if (m_inputs[i]) {
      Disconnect(m_inputs[i]);
    }
  }

  err = AUGraphRemoveNode(GlobalAUGraph::GetAUGraph(), m_mixerNode);
  ASSERTVALUE(err == noErr, err);

  m_mixerNode = 0;
  m_mixerUnit = 0;
}

OSStatus Mixer::Initialize(OSType mixerType) {
  OSStatus                  err;
  AudioComponentDescription desc;
  UInt32                    reverb;

  ASSERT(m_mixerNode == 0);
  ASSERT(m_mixerUnit == 0);

  m_mixerType = mixerType;

  ASSERTVALUE(mixerType == kAudioUnitSubType_3DMixer || mixerType == kAudioUnitSubType_StereoMixer, mixerType);

  desc.componentType = kAudioUnitType_Mixer;
  desc.componentSubType = mixerType;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;
  err = AUGraphAddNode(GlobalAUGraph::GetAUGraph(), &desc, &m_mixerNode);
  ASSERTVALUE(err == noErr, err);

  err = AUGraphNodeInfo(GlobalAUGraph::GetAUGraph(), m_mixerNode, 0, &m_mixerUnit);
  ASSERTVALUE(err == noErr, err);

  err = AudioUnitInitialize(m_mixerUnit);
  ASSERTVALUE(err == noErr, err);

  if (mixerType == kAudioUnitSubType_3DMixer) {
    reverb = 0;
    err = AudioUnitSetProperty(m_mixerUnit, kAudioUnitProperty_UsesInternalReverb, kAudioUnitScope_Global, 0, &reverb, sizeof(reverb));
    ASSERTVALUE(err == noErr, err);
  }

  return err;
}

OSStatus Mixer::Connect(AUNode node, int sourceOutput) {
  OSStatus err;
  int      i;

  ASSERT(m_mixerNode != 0);

  m_lock.Enter();

  for (i = 0; i < MAX_INPUTS; ++i) {
    if (!m_inputs[i]) {
      break;
    }
  }

  if (i == MAX_INPUTS) {
    m_lock.Leave();
    return kAUGraphErr_InvalidConnection;
  }

  m_inputs[i] = node;

  err = AUGraphConnectNodeInput(GlobalAUGraph::GetAUGraph(), node, sourceOutput, m_mixerNode, i);
  ASSERTVALUE(err == noErr, err);

  m_nameLabels[i] = "new connection";
  m_stateLabels[i] = "????";

  m_lock.Leave();
  return err;
}

OSStatus Mixer::Disconnect(AUNode node) {
  OSStatus err;
  int      i;

  m_lock.Enter();

  for (i = 0; i < MAX_INPUTS; ++i) {
    if (m_inputs[i] == node) {
      break;
    }
  }

  if (i == MAX_INPUTS) {
    m_lock.Leave();
    return kAUGraphErr_NodeNotFound;
  }

  err = AUGraphDisconnectNodeInput(GlobalAUGraph::GetAUGraph(), m_mixerNode, i);
  ASSERTVALUE(err == noErr, err);

  m_nameLabels[i] = "disconnected";
  m_stateLabels[i] = "----";
  m_inputs[i] = 0;

  m_lock.Leave();
  return err;
}

OSStatus Mixer::SetMixVolume(AUNode node, float volume) {
  OSStatus err;
  float    gain;
  int      i;

  for (i = 0; i < MAX_INPUTS; ++i) {
    if (m_inputs[i] == node) {
      break;
    }
  }

  if (i == MAX_INPUTS) {
    return kAUGraphErr_NodeNotFound;
  }

  if (m_mixerType == kAudioUnitSubType_StereoMixer) {
    err = AudioUnitSetParameter(m_mixerUnit, kStereoMixerParam_Volume, kAudioUnitScope_Input, i, volume, 0);
  } else if (m_mixerType == kAudioUnitSubType_3DMixer) {
    gain = volume > 0.0f ? 14.426950454711914 * log(volume) : -120.0f;
    err = AudioUnitSetParameter(m_mixerUnit, k3DMixerParam_Gain, kAudioUnitScope_Input, i, gain, 0);
  } else {
    err = kAUGraphErr_InvalidAudioUnit;
  }

  return err;
}

OSStatus Mixer::Set3DDistance(AUNode node, float distance) {
  int i;

  if (m_mixerType != kAudioUnitSubType_3DMixer) {
    return kAUGraphErr_InvalidAudioUnit;
  }

  for (i = 0; i < MAX_INPUTS; ++i) {
    if (m_inputs[i] == node) {
      break;
    }
  }

  if (i == MAX_INPUTS) {
    return kAUGraphErr_NodeNotFound;
  }

  return AudioUnitSetParameter(m_mixerUnit, k3DMixerParam_Distance, kAudioUnitScope_Input, i, distance, 0);
}

OSStatus Mixer::Set3DAzimuth(AUNode node, float azimuth) {
  int i;

  if (m_mixerType != kAudioUnitSubType_3DMixer) {
    return kAUGraphErr_InvalidAudioUnit;
  }

  for (i = 0; i < MAX_INPUTS; ++i) {
    if (m_inputs[i] == node) {
      break;
    }
  }

  if (i == MAX_INPUTS) {
    return kAUGraphErr_NodeNotFound;
  }

  return AudioUnitSetParameter(m_mixerUnit, k3DMixerParam_Azimuth, kAudioUnitScope_Input, i, azimuth, 0);
}

OSStatus Mixer::Set3DElevation(AUNode node, float elevation) {
  int i;

  if (m_mixerType != kAudioUnitSubType_3DMixer) {
    return kAUGraphErr_InvalidAudioUnit;
  }

  for (i = 0; i < MAX_INPUTS; ++i) {
    if (m_inputs[i] == node) {
      break;
    }
  }

  if (i == MAX_INPUTS) {
    return kAUGraphErr_NodeNotFound;
  }

  return AudioUnitSetParameter(m_mixerUnit, k3DMixerParam_Elevation, kAudioUnitScope_Input, i, elevation, 0);
}

void Mixer::LabelNode(AUNode node, const char *name, const char *state) {
  for (int i = 0; i < MAX_INPUTS; ++i) {
    if (m_inputs[i] == node) {
      m_nameLabels[i] = name;
      m_stateLabels[i] = state;
      return;
    }
  }
}

void Mixer::Dump(MixerDumpLevel level) {
  if (!level) {
    return;
  }

  printf("\n\n");

  for (int i = 0; i < MAX_INPUTS; ++i) {
    if (!m_inputs[i]) {
      printf("Channel %2d: -\n", i);
    } else {
      printf("Channel %2d: %s %s\n", i, m_stateLabels[i], m_nameLabels[i]);
    }
  }
}
