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

typedef int(*ANIMSEQFINISHEDHANDLER)(void *);
typedef int(*ANIMBONEPROJECTCALLBACK)(const NTempest::C3Segment &segment, float &distance);

HANIM AnimCreate(unsigned char *fileData, unsigned int fileBytes, unsigned int flags);
HANIM AnimDuplicate(HANIM oldanim, unsigned int flags);
unsigned int AnimBuildObjectIdTranslation(
    unsigned char *fileData,
    unsigned int   fileBytes,
    unsigned int   flags,
    unsigned int  *idConversion,
    unsigned int   numObjects
);

BOOL AnimIsAttachmentEnabled(HANIM anim, unsigned int index);
int AnimIsCameraEnabled(HANIM anim, unsigned int index);
int AnimHasObjectId(HANIM anim, unsigned int objectId);
int AnimUsesBlending(HANIM anim);
void AnimEnumObjects(HANIM anim, int(*callbackfcn)(unsigned int, const char *, void *), void *param);
int AnimForceSequenceTime(HANIM anim, unsigned int index, int time);
int AnimForceCurrentSequenceTime(HANIM anim, int time);
unsigned int AnimGetElapsedTime();
int AnimAdvanceTime(HANIM anim, unsigned int currentFrame);
int AnimManualAdvanceTime(HANIM anim, int timeChange);
void AnimPauseTime(HANIM anim, int pause);
void AnimResetGlobalSequenceTimes(HANIM anim);
int AnimSetSequence(HANIM anim, unsigned int seqIndex, unsigned int flags);
int AnimSetSequence(HANIM anim, unsigned int seqIndex, unsigned int objectId, unsigned int flags);
int AnimMatchSequence(HANIM anim, unsigned int objectId, unsigned int sameAsObjectId, unsigned int flags);
int AnimSetRandomSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int flags);
int AnimSetRandomSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int objectId, unsigned int flags);
int AnimSetSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int fidgetId, unsigned int flags);
int AnimSetSequenceFidget(HANIM anim, unsigned int seqIndex, unsigned int fidgetId, unsigned int objectId, unsigned int flags);
unsigned int AnimGetNumSequenceFidgets(HANIM anim, unsigned int seqIndex);
unsigned int AnimGetNumSequences(HANIM anim);
int AnimGetSequenceDuration(HANIM anim, unsigned int seqIndex, unsigned int *duration);
int AnimGetSequenceTime(HANIM anim, unsigned int seqIndex);
int AnimGetSequenceMoveSpeed(HANIM anim, unsigned int seqIndex, float *moveSpeed);
int AnimGetSequenceName(HANIM anim, unsigned int seqIndex, char *buffer, unsigned int buffLength);
int AnimHasSequenceId(HANIM anim, unsigned int seqIndex);
unsigned int AnimGetTotalKeys(HANIM anim);
void AnimSetTimeScale(HANIM anim, float timeScale);
float AnimGetTimeScale(HANIM anim);
float AnimGetObjectTimeScale(HANIM anim, unsigned int objectId);
unsigned int AnimGetFlags(HANIM anim);
void AnimEnableBlending(HANIM anim, int enable);
int AnimSetObjectTimeScale(HANIM anim, unsigned int objectId, float timeScale);
int AnimApplyObjectLookAt(HANIM anim, unsigned int objectId, const NTempest::C3Vector &target);
int AnimRemoveObjectLookAt(HANIM anim, unsigned int objectId);
int AnimObjectUsingLookAt(HANIM anim, unsigned int objectId);
int AnimApplyObjectFaceDir(HANIM anim, unsigned int objectId, NTempest::C3Vector direction);
int AnimRemoveObjectFaceDir(HANIM anim, unsigned int objectId);
int AnimObjectUsingFaceDir(HANIM anim, unsigned int objectId);
int AnimMarkFootstepSequence(HANIM anim, unsigned int index);
int AnimLockObjectSequence(HANIM anim, unsigned int objectId, int set);
float AnimGetPrimarySequenceCompletion(HANIM anim);
int AnimEventEmitterHasKeysThisSeq(HANIM anim, unsigned int objectId);
int
AnimGetObjectPosition(HANIM anim, unsigned int objectId, const TSFixedArray<NTempest::C3Vector> &positions, NTempest::C3Vector *position);
int AnimGetEventObjectPosition(HANIM anim, unsigned int objectId, NTempest::C3Vector *position);
void AnimAnimateModel(HANIM anim, const CAnimationData &data);
void AnimAnimateCameras(HANIM anim, const TSFixedArray<HCAMERA> &cameras);
void AnimProcessEvents(HANIM anim, const TSFixedArray<NTempest::C3Vector> &positions);
int AnimGetPrimarySequence(HANIM anim, unsigned int *sequence);
int AnimNeedsSequenceBounds(HANIM anim);
void AnimSetSeqFinishedHandler(HANIM anim, ANIMSEQFINISHEDHANDLER callback, void *param);
void AnimSetSeqFinishedHandler(HANIM anim, unsigned int sequence, ANIMSEQFINISHEDHANDLER callback, void *param);
void AnimSetEventCallback(HANIM anim, void(*callback)(const char *, const NTempest::C3Vector &, void *), void *param);
void AnimSetBoneProjectCallback(ANIMBONEPROJECTCALLBACK callback, float distance);
void AnimGetBoneProjectCallback(ANIMBONEPROJECTCALLBACK &callback, float &distance);
void AnimSetObjectOrdering(HANIM anim, const char **boneNames, unsigned int numBones);
void AnimSetSequenceOrderingDefault(HANIM anim);
void AnimSetSequenceOrdering(HANIM anim, const char **sequenceNames, unsigned int numSequences);
void AnimSetCameraOrdering(HANIM anim, const char **cameraNames, unsigned int numCameras, TSFixedArray<unsigned int> *cameraOrder);
void AnimResetCameraOrdering(HANIM anim, TSFixedArray<unsigned int> *cameraOrder);
