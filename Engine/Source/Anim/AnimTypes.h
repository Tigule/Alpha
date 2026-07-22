#pragma once

#include "Base/Handle.h"
#include "Services/Camera.h"
#include "Tempest/cimvector.h"

#include <stddef.h>

template <class T>
class TSFixedArray;

class CParticleEmitter2;
class CRibbonEmitter;

struct CGeosetColor {
  CGeosetColor() : animatedColor(0xFFFFFFFF), proceduralColor(0xFFFFFFFF), animatedAlpha(1.0f), proceduralAlpha(1.0f) {
  }

  NTempest::CImVector animatedColor;
  NTempest::CImVector proceduralColor;
  float               animatedAlpha;
  float               proceduralAlpha;
};

DECLARE_DERIVED_HANDLE(HANIM, HOBJECT);
DECLARE_DERIVED_HANDLE(HANIMDATA, HOBJECT);

namespace NTempest {
  class C3Segment;
  class C3Vector;
  class C34Matrix;
}  // namespace NTempest

struct CAnimationData {
  NTempest::C34Matrix               *boneMtx;
  unsigned int                       numBones;
  NTempest::C34Matrix               *textureMtx;
  unsigned int                       numTexBones;
  TSFixedArray<NTempest::C3Vector>  *positions;
  TSFixedArray<unsigned long>       *lights;
  TSFixedArray<CParticleEmitter2 *> *emitters2;
  TSFixedArray<CRibbonEmitter *>    *ribbons;
  NTempest::C34Matrix               *attached;
  unsigned int                       numAttached;
  CGeosetColor                      *geosetColor;
  const NTempest::C3Vector          *cameraWorldPos;
  const NTempest::C3Vector          *cameraVector;
  unsigned char                     *layerAlpha;
  unsigned int                      *layerTextureIds;
};

typedef int(__fastcall *ANIMSEQFINISHEDHANDLER)(void *);
typedef int(__fastcall *ANIMBONEPROJECTCALLBACK)(const NTempest::C3Segment &segment, float &distance);

HANIM __fastcall        AnimCreate(unsigned char *fileData, unsigned int fileBytes, unsigned int flags);
HANIM __fastcall        AnimDuplicate(HANIM oldanim, unsigned int flags);
unsigned int __fastcall AnimBuildObjectIdTranslation(
    unsigned char *fileData,
    unsigned int   fileBytes,
    unsigned int   flags,
    unsigned int  *idConversion,
    unsigned int   numObjects
);

BOOL __fastcall         AnimIsAttachmentEnabled(HANIM anim, unsigned int index);
int __fastcall          AnimIsCameraEnabled(HANIM anim, unsigned int index);
int __fastcall          AnimHasObjectId(HANIM anim, unsigned int objectId);
int __fastcall          AnimUsesBlending(HANIM anim);
void __fastcall         AnimEnumObjects(HANIM anim, int(__fastcall *callbackfcn)(unsigned int, const char *, void *), void *param);
int __fastcall          AnimForceSequenceTime(HANIM anim, unsigned int index, int time);
int __fastcall          AnimForceCurrentSequenceTime(HANIM anim, int time);
unsigned int __fastcall AnimGetElapsedTime();
int __fastcall          AnimAdvanceTime(HANIM anim, unsigned int currentFrame);
int __fastcall          AnimManualAdvanceTime(HANIM anim, int timeChange);
void __fastcall         AnimPauseTime(HANIM anim, int pause);
void __fastcall         AnimResetGlobalSequenceTimes(HANIM anim);
int __fastcall          AnimSetSequence(HANIM anim, unsigned int seqIndex, unsigned int flags);
int __fastcall          AnimSetSequence(HANIM anim, unsigned int seqIndex, unsigned int objectId, unsigned int flags);
int __fastcall          AnimMatchSequence(HANIM anim, unsigned int objectId, unsigned int sameAsObjectId, unsigned int flags);
int __fastcall          AnimSetRandomSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int flags);
int __fastcall          AnimSetRandomSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int objectId, unsigned int flags);
int __fastcall          AnimSetSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int fidgetId, unsigned int flags);
int __fastcall          AnimSetSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int fidgetId, unsigned int objectId, unsigned int flags);
unsigned int __fastcall AnimGetNumSequenceFidgets(HANIM anim, unsigned int seqIndex);
unsigned int __fastcall AnimGetNumSequences(HANIM anim);
int __fastcall          AnimGetSequenceDuration(HANIM anim, unsigned int seqIndex, unsigned int *duration);
int __fastcall          AnimGetSequenceTime(HANIM anim, unsigned int seqIndex);
int __fastcall          AnimGetSequenceMoveSpeed(HANIM anim, unsigned int seqIndex, float *moveSpeed);
int __fastcall          AnimGetSequenceName(HANIM anim, unsigned int seqIndex, char *buffer, unsigned int buffLength);
int __fastcall          AnimHasSequenceId(HANIM anim, unsigned int seqIndex);
unsigned int __fastcall AnimGetTotalKeys(HANIM anim);
void __fastcall         AnimSetTimeScale(HANIM anim, float timeScale);
float __fastcall        AnimGetTimeScale(HANIM anim);
float __fastcall        AnimGetObjectTimeScale(HANIM anim, unsigned int objectId);
unsigned int __fastcall AnimGetFlags(HANIM anim);
void __fastcall         AnimEnableBlending(HANIM anim, int enable);
int __fastcall          AnimSetObjectTimeScale(HANIM anim, unsigned int objectId, float timeScale);
int __fastcall          AnimApplyObjectLookAt(HANIM anim, unsigned int objectId, const NTempest::C3Vector &target);
int __fastcall          AnimRemoveObjectLookAt(HANIM anim, unsigned int objectId);
int __fastcall          AnimObjectUsingLookAt(HANIM anim, unsigned int objectId);
int __fastcall          AnimApplyObjectFaceDir(HANIM anim, unsigned int objectId, NTempest::C3Vector direction);
int __fastcall          AnimRemoveObjectFaceDir(HANIM anim, unsigned int objectId);
int __fastcall          AnimObjectUsingFaceDir(HANIM anim, unsigned int objectId);
int __fastcall          AnimMarkFootstepSequence(HANIM anim, unsigned int index);
int __fastcall          AnimLockObjectSequence(HANIM anim, unsigned int objectId, int set);
float __fastcall        AnimGetPrimarySequenceCompletion(HANIM anim);
int __fastcall          AnimEventEmitterHasKeysThisSeq(HANIM anim, unsigned int objectId);
int __fastcall
AnimGetObjectPosition(HANIM anim, unsigned int objectId, const TSFixedArray<NTempest::C3Vector> &positions, NTempest::C3Vector *position);
int __fastcall  AnimGetEventObjectPosition(HANIM anim, unsigned int objectId, NTempest::C3Vector *position);
void __fastcall AnimAnimateModel(HANIM anim, const CAnimationData &data);
void __fastcall AnimAnimateCameras(HANIM anim, const TSFixedArray<HCAMERA> &cameras);
void __fastcall AnimProcessEvents(HANIM anim, const TSFixedArray<NTempest::C3Vector> &positions);
int __fastcall  AnimGetPrimarySequence(HANIM anim, unsigned int *sequence);
int __fastcall  AnimNeedsSequenceBounds(HANIM anim);
void __fastcall AnimSetSeqFinishedHandler(HANIM anim, ANIMSEQFINISHEDHANDLER callback, void *param);
void __fastcall AnimSetSeqFinishedHandler(HANIM anim, unsigned int sequence, ANIMSEQFINISHEDHANDLER callback, void *param);
void __fastcall AnimSetEventCallback(HANIM anim, void(__fastcall *callback)(const char *, const NTempest::C3Vector &, void *), void *param);
void __fastcall AnimSetBoneProjectCallback(ANIMBONEPROJECTCALLBACK callback, float distance);
void __fastcall AnimGetBoneProjectCallback(ANIMBONEPROJECTCALLBACK &callback, float &distance);
void __fastcall AnimSetObjectOrdering(HANIM anim, const char **boneNames, unsigned int numBones);
void __fastcall AnimSetSequenceOrderingDefault(HANIM anim);
void __fastcall AnimSetSequenceOrdering(HANIM anim, const char **sequenceNames, unsigned int numSequences);
void __fastcall AnimSetCameraOrdering(HANIM anim, const char **cameraNames, unsigned int numCameras, TSFixedArray<unsigned int> *cameraOrder);
