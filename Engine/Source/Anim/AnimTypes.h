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
  UINT                               numBones;
  NTempest::C34Matrix               *textureMtx;
  UINT                               numTexBones;
  TSFixedArray<NTempest::C3Vector>  *positions;
  TSFixedArray<DWORD>               *lights;
  TSFixedArray<CParticleEmitter2 *> *emitters2;
  TSFixedArray<CRibbonEmitter *>    *ribbons;
  NTempest::C34Matrix               *attached;
  UINT                               numAttached;
  CGeosetColor                      *geosetColor;
  const NTempest::C3Vector          *cameraWorldPos;
  const NTempest::C3Vector          *cameraVector;
  BYTE                              *layerAlpha;
  UINT                              *layerTextureIds;
};

typedef BOOL (*ANIMSEQFINISHEDHANDLER)(LPVOID);
typedef int (*ANIMBONEPROJECTCALLBACK)(const NTempest::C3Segment &segment, float &distance);

HANIM AnimCreate(BYTE *fileData, UINT fileBytes, UINT flags);
HANIM AnimDuplicate(HANIM oldanim, UINT flags);
UINT  AnimBuildObjectIdTranslation(BYTE *fileData, UINT fileBytes, UINT flags, UINT *idConversion, UINT numObjects);

UINT  AnimGetAttachmentObjId(HANIM anim, UINT index);
BOOL  AnimIsAttachmentEnabled(HANIM anim, UINT index);
int   AnimIsCameraEnabled(HANIM anim, UINT index);
BOOL  AnimHasObjectId(HANIM anim, UINT objectId);
int   AnimUsesBlending(HANIM anim);
void  AnimEnumObjects(HANIM anim, int (*callbackfcn)(UINT, LPCSTR, LPVOID), LPVOID param);
int   AnimForceSequenceTime(HANIM anim, UINT index, int time);
int   AnimForceCurrentSequenceTime(HANIM anim, int time);
UINT  AnimGetElapsedTime();
int   AnimAdvanceTime(HANIM anim, UINT currentFrame);
int   AnimManualAdvanceTime(HANIM anim, int timeChange);
void  AnimPauseTime(HANIM anim, int pause);
void  AnimResetGlobalSequenceTimes(HANIM anim);
BOOL  AnimSetSequence(HANIM anim, UINT seqIndex, UINT flags);
BOOL  AnimSetSequence(HANIM anim, UINT seqIndex, UINT objectId, UINT flags);
BOOL  AnimMatchSequence(HANIM anim, UINT objectId, UINT sameAsObjectId, UINT flags);
BOOL  AnimSetRandomSequenceFidget(HANIM anim, UINT seqIndex, UINT flags);
BOOL  AnimSetRandomSequenceFidget(HANIM anim, UINT seqIndex, UINT objectId, UINT flags);
BOOL  AnimSetSequenceFidget(HANIM anim, UINT seqIndex, UINT fidgetId, UINT flags);
BOOL  AnimSetSequenceFidget(HANIM anim, UINT seqIndex, UINT fidgetId, UINT objectId, UINT flags);
UINT  AnimGetNumSequenceFidgets(HANIM anim, UINT seqIndex);
UINT  AnimGetNumSequences(HANIM anim);
BOOL  AnimGetSequenceDuration(HANIM anim, UINT seqIndex, UINT *duration);
int   AnimGetSequenceTime(HANIM anim, UINT seqIndex);
BOOL  AnimGetSequenceMoveSpeed(HANIM anim, UINT seqIndex, float *moveSpeed);
BOOL  AnimGetSequenceName(HANIM anim, UINT seqIndex, char *buffer, UINT buffLength);
BOOL  AnimHasSequenceId(HANIM anim, UINT seqIndex);
UINT  AnimGetTotalKeys(HANIM anim);
void  AnimSetTimeScale(HANIM anim, float timeScale);
float AnimGetTimeScale(HANIM anim);
float AnimGetObjectTimeScale(HANIM anim, UINT objectId);
UINT  AnimGetFlags(HANIM anim);
void  AnimEnableBlending(HANIM anim, int enable);
BOOL  AnimSetObjectTimeScale(HANIM anim, UINT objectId, float timeScale);
int   AnimApplyObjectLookAt(HANIM anim, UINT objectId, const NTempest::C3Vector &target);
int   AnimRemoveObjectLookAt(HANIM anim, UINT objectId);
int   AnimObjectUsingLookAt(HANIM anim, UINT objectId);
int   AnimApplyObjectFaceDir(HANIM anim, UINT objectId, NTempest::C3Vector direction);
int   AnimRemoveObjectFaceDir(HANIM anim, UINT objectId);
int   AnimObjectUsingFaceDir(HANIM anim, UINT objectId);
BOOL  AnimMarkFootstepSequence(HANIM anim, UINT index);
BOOL  AnimLockObjectSequence(HANIM anim, UINT objectId, int set);
float AnimGetPrimarySequenceCompletion(HANIM anim);
BOOL  AnimEventEmitterHasKeysThisSeq(HANIM anim, UINT objectId);
BOOL  AnimGetObjectPosition(HANIM anim, UINT objectId, const TSFixedArray<NTempest::C3Vector> &positions, NTempest::C3Vector *position);
BOOL  AnimGetEventObjectPosition(HANIM anim, UINT objectId, NTempest::C3Vector *position);
void  AnimAnimateModel(HANIM anim, const CAnimationData &data);
void  AnimAnimateCameras(HANIM anim, const TSFixedArray<HCAMERA> &cameras);
void  AnimProcessEvents(HANIM anim, const TSFixedArray<NTempest::C3Vector> &positions);
BOOL  AnimGetPrimarySequence(HANIM anim, UINT *sequence);
BOOL  AnimNeedsSequenceBounds(HANIM anim);
void  AnimSetSeqFinishedHandler(HANIM anim, ANIMSEQFINISHEDHANDLER callback, LPVOID param);
void  AnimSetSeqFinishedHandler(HANIM anim, UINT sequence, ANIMSEQFINISHEDHANDLER callback, LPVOID param);
void  AnimSetEventCallback(HANIM anim, void (*callback)(LPCSTR, const NTempest::C3Vector &, LPVOID), LPVOID param);
void  AnimSetBoneProjectCallback(ANIMBONEPROJECTCALLBACK callback, float distance);
void  AnimGetBoneProjectCallback(ANIMBONEPROJECTCALLBACK &callback, float &distance);
void  AnimSetObjectOrdering(HANIM anim, LPCSTR *boneNames, UINT numBones);
void  AnimSetSequenceOrderingDefault(HANIM anim);
void  AnimSetSequenceOrdering(HANIM anim, LPCSTR *sequenceNames, UINT numSequences);
void  AnimSetCameraOrdering(HANIM anim, LPCSTR *cameraNames, UINT numCameras, TSFixedArray<UINT> *cameraOrder);
void  AnimResetCameraOrdering(HANIM anim, TSFixedArray<UINT> *cameraOrder);
