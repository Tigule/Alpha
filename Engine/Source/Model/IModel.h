#pragma once

#include "Base/Handle.h"
#include "Gx/Gx.h"
#include "Services/Camera.h"

class CStatus;
class CGxLight;
struct HTEXTURE__;
typedef HTEXTURE__ *HTEXTURE;

struct CModelCreate {
  unsigned int flags;
  const char **sequenceNames;
  unsigned int numSequences;
  const char **boneNames;
  unsigned int numBones;
  const char **cameraNames;
  unsigned int numCameras;
};

namespace NTempest {
  class CAaBox;
  class CAaSphere;
  class C34Matrix;
  class C3Vector;
  class C44Matrix;
  class CImVector;
}  // namespace NTempest

typedef void(__fastcall *MODELPROJECT2DCALLBACK)(NTempest::CAaBox &bounds, NTempest::CImVector color, NTempest::C44Matrix &view);

DECLARE_DERIVED_HANDLE(HMODEL, HOBJECT);

int __fastcall ModelGetEventObjectPosition(HMODEL model, unsigned int objectId, int modelSpace, NTempest::C3Vector *position);

void __fastcall ModelInitialize();
void __fastcall ModelDestroy();
void __fastcall ModelCacheFlush();
void __fastcall ModelRemoveFromCache(const char *sourcefile);
int __fastcall  ModelCacheUpdate(unsigned long currentTime, CStatus *status);
int __fastcall  ModelIsLoaded(HMODEL modelHandle, int doLinkedModels);
void __fastcall ModelProcessEvents(HMODEL model, const NTempest::C34Matrix &orientation);
void __fastcall
ModelProcessEvents(HMODEL model, const NTempest::C3Vector &position, float rotationAngle, const NTempest::C3Vector &rotationAxis, float scale);
void __fastcall ModelGetStandingMatrix(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    const NTempest::C3Vector &groundNormal,
    float                     facing,
    float                     scale,
    NTempest::C34Matrix      *orientation
);
void __fastcall ModelForceStandingMatrix(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    const NTempest::C3Vector &groundNormal,
    float                     facing,
    float                     scale,
    int                       enumGroundTrack,
    float                     blendRatio,
    NTempest::C34Matrix      *orientation
);
int __fastcall          ModelAdvanceTime(HMODEL model);
int __fastcall          ModelSetSequence(HMODEL model, unsigned int seqIndex, unsigned int flags);
int __fastcall          ModelSetSequence(HMODEL model, unsigned int seqIndex, unsigned int objectId, unsigned int flags);
int __fastcall          ModelMatchSequence(HMODEL model, unsigned int objectId, unsigned int sameAsObjectId, unsigned int flags);
int __fastcall          ModelSetRandomSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int flags);
int __fastcall          ModelSetRandomSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int objectId, unsigned int flags);
int __fastcall          ModelSetSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int fidgetId, unsigned int flags);
int __fastcall          ModelSetSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int fidgetId, unsigned int objectId, unsigned int flags);
unsigned int __fastcall ModelGetNumSequenceFidgets(HMODEL model, unsigned int seqIndex);
int __fastcall          ModelGetSequenceDuration(HMODEL model, unsigned int seqIndex, unsigned int *duration);
int __fastcall          ModelGetSequenceTime(HMODEL model, unsigned int seqIndex);
int __fastcall          ModelHasSequenceId(HMODEL model, unsigned int seqIndex);
HMODEL __fastcall       ModelCreate(const char *sourcefile, CModelCreate *data, CStatus *status);
HMODEL __fastcall       ModelCreateSolidSphere(float radius, HTEXTURE texture);
HMODEL __fastcall       ModelCreateBox(const NTempest::CAaBox &bounds, HTEXTURE texture, EGxBlend blendMode);
HMODEL __fastcall       ModelDuplicate(HMODEL sourceModel, unsigned int flags);
int __fastcall          ModelAddLink(HMODEL parent, unsigned int parentIndex, HMODEL child, float scale);
int __fastcall          ModelClearLink(HMODEL parent, unsigned int parentIndex);
int __fastcall          ModelRemoveLink(HMODEL parent, unsigned int parentIndex, HMODEL child);
void __fastcall         ModelClearAllLinks(HMODEL parent);
int __fastcall          ModelGetLinkPoint(HMODEL model, unsigned int index, HMODEL *modelList, unsigned int *entriesInOut);
int __fastcall          ModelGetNumLinkedAtPoint(HMODEL model, unsigned int index, unsigned int *numLinked);
int __fastcall          ModelHasLinkPoint(HMODEL model, unsigned int index);
unsigned int __fastcall ModelGetNumLinkPoints(HMODEL model);
int __fastcall          ModelReplaceTexture(HMODEL model, unsigned int replaceableId, HTEXTURE texture, int doLinkedModels);
int __fastcall          ModelGetExtents(HMODEL model, NTempest::CAaBox *extents);
int __fastcall          ModelGetSeqExtents(HMODEL model, unsigned int seqnum, NTempest::CAaBox *extents);
int __fastcall          ModelGetBounds(HMODEL model, NTempest::CAaSphere *bounds);
void __fastcall         ModelSceneCalcFrustumPlanes();
int __fastcall          ModelTestSphere(HMODEL model, NTempest::C34Matrix &orientation, float scale, int testLinkedModels);
int __fastcall ModelHitTestSphere(HMODEL model, float scale, NTempest::C3Vector &a, NTempest::C3Vector &b, int testLinkedModels, float *linePos);
int __fastcall ModelHasHitTestVolumes(HMODEL model);
int __fastcall ModelHitTestVolumes(HMODEL model, float scale, NTempest::C3Vector &a, NTempest::C3Vector &b, int testLinkedModels, float *linePos);
int __fastcall ModelHitTestGeometry(HMODEL model, float scale, NTempest::C3Vector &a, NTempest::C3Vector &b, int testLinkedModels, float *linePos);
int __fastcall ModelGetModelSpacePivot(HMODEL model, unsigned int objectId, NTempest::C3Vector *pivot);
HCAMERA __fastcall         ModelGetCamera(HMODEL model, unsigned int index);
unsigned int __fastcall    ModelGetNumCameras(HMODEL model);
int __fastcall             ModelIsCameraEnabled(HMODEL model, unsigned int index);
int __fastcall             ModelIsShowingBoundingSphere(HMODEL model);
int __fastcall             ModelIsShowingBoundingBox(HMODEL model);
int __fastcall             ModelIsShowingHitTestGeometry(HMODEL model);
void __fastcall            ModelRestoreBlendMode(HMODEL model, int doLinkedModels);
void __fastcall            ModelSetBlendMode(HMODEL model, EGxBlend blendMode, int doLinkedModels);
unsigned int __fastcall    ModelGetNumLights(HMODEL model);
const CGxLight *__fastcall ModelGetLight(HMODEL model, unsigned int index);
void __fastcall            ModelEnableFullAlpha(HMODEL model, int enable);
void __fastcall            ModelEnableAnimBlending(HMODEL model, int enabled);
void __fastcall            ModelSetEmissiveColor(HMODEL model, const NTempest::CImVector &color, int doLinkedModels);
void __fastcall            ModelHideBounds(HMODEL model);
void __fastcall            ModelHideGeosets(HMODEL model, unsigned int selectionGroup, int hide);
void __fastcall            ModelHideGeosetsRange(HMODEL model, unsigned int selectionStart, unsigned int selectionEnd, int hide);
int __fastcall             ModelOptimizeVisibleGeosets(HMODEL model);
void __fastcall            ModelShowCollision(HMODEL model, int show);
void __fastcall            ModelShowCollisionAaBox(HMODEL model, int show);
void __fastcall            ModelShowModel(HMODEL model, int show);
void __fastcall            ModelSetProject2dCallback(MODELPROJECT2DCALLBACK callback);
void __fastcall            ModelScenePlaceCamera(const NTempest::C3Vector &position, const NTempest::C3Vector &direction);
void __fastcall            ModelSceneSetSharpness(float sharpness);
void __fastcall            ModelSetLightSelectCallback(
    HMODEL model,
    void(__fastcall *callback)(void *, NTempest::C3Vector, const NTempest::C3Vector &, unsigned int),
    void *parm,
    int   doLinkedModels
);
void __fastcall
ModelSetEventCallback(HMODEL model, void(__fastcall *callback)(const char *, const NTempest::C3Vector &, void *), void *param, int doLinkedModels);
int __fastcall          ModelForceSequenceTime(HMODEL model, unsigned int seqIndex, int timeOffset, int doLinkedModels);
int __fastcall          ModelForceCurrentSequenceTime(HMODEL model, int timeOffset, int doLinkedModels);
int __fastcall          ModelGetSequenceMoveSpeed(HMODEL model, unsigned int seqIndex, float *moveSpeed);
int __fastcall          ModelGetObjectPosition(HMODEL model, unsigned int objectId, NTempest::C3Vector *position);
void __fastcall         ModelSetSeqFinishedHandler(HMODEL model, int(__fastcall *callback)(void *), void *param);
void __fastcall         ModelSetSeqFinishedHandler(HMODEL model, unsigned int sequence, int(__fastcall *callback)(void *), void *param);
void __fastcall         ModelSetTimeScale(HMODEL model, float timeScale, int doLinkedModels);
int __fastcall          ModelSetObjectTimeScale(HMODEL model, unsigned int objectId, float timeScale, int doLinkedModels);
int __fastcall          ModelApplyObjectLookAt(HMODEL model, unsigned int objectId, const NTempest::C3Vector &target);
int __fastcall          ModelRemoveObjectLookAt(HMODEL model, unsigned int objectId);
int __fastcall          ModelApplyObjectFaceDir(HMODEL model, unsigned int objectId, const NTempest::C3Vector &direction);
int __fastcall          ModelRemoveObjectFaceDir(HMODEL model, unsigned int objectId);
int __fastcall          ModelMarkFootstepSequence(HMODEL model, unsigned int seqIndex);
int __fastcall          ModelLockObjectSequence(HMODEL model, unsigned int objectId, int set);
void __fastcall         ModelSetVertexColor(HMODEL model, unsigned int red, unsigned int green, unsigned int blue, int doLinkedModels);
void __fastcall         ModelGetVertexColor(HMODEL model, unsigned int &red, unsigned int &green, unsigned int &blue);
void __fastcall         ModelShowUnselectable(HMODEL model, unsigned char red, unsigned char green, unsigned char blue);
void __fastcall         ModelHideUnselectable(HMODEL model);
int __fastcall          ModelIsShowingUnselectable(HMODEL model);
void __fastcall         ModelSetVertexAlpha(HMODEL model, unsigned int alpha, int doLinkedModels);
unsigned int __fastcall ModelGetVertexAlpha(HMODEL model);
void __fastcall         ModelCustGeosetMove(HMODEL model, unsigned int custGeosetId, const NTempest::C3Vector &modelSpacePosition);
unsigned int __fastcall ModelGetPrimarySequence(HMODEL model);
int __fastcall          ModelUsesBlending(HMODEL model);
void __fastcall         ModelEnumAnimObjects(HMODEL model, int(__fastcall *callbackfcn)(unsigned int, const char *, void *), void *param);
void __fastcall         ModelEnableEmitters(HMODEL model, int enable, int doLinkedModels);
void __fastcall         ModelEnableRibbons(HMODEL model, int enable);
int __fastcall          ModelAnimHasObjectId(HMODEL model, unsigned int objectId);
unsigned int __fastcall ModelGetNumTextures(HMODEL model);
unsigned int __fastcall ModelGetTextureReplaceableId(HMODEL model, unsigned int textureId);
void __fastcall         ModelAnimate(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    float                     scale,
    const NTempest::C3Vector &cameraWorldPos,
    const NTempest::C3Vector &cameraVector
);
void __fastcall ModelAnimateCameras(HMODEL model, const NTempest::C34Matrix &orientation);
void __fastcall ModelResetGlobalSequenceTimes(HMODEL model, int doLinkedModels);
void __fastcall ModelAnimate(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    const NTempest::C3Vector &nonUniformScale,
    const NTempest::C3Vector &cameraWorldPos,
    const NTempest::C3Vector &cameraVector
);
void __fastcall ModelAnimate(
    HMODEL                     model,
    const NTempest::C34Matrix &orientation,
    float                      scale,
    const NTempest::C3Vector  &cameraWorldPos,
    const NTempest::C3Vector  &cameraVector
);
void __fastcall ModelAddToScene(HMODEL model, unsigned int renderFlags);
void __fastcall ModelAddToScene(NTempest::C3Vector &position, int priorityPlane, void(__fastcall *callback)(void *, int), void *param1, int param2);
void __fastcall ModelRender(HMODEL model, CStatus *status, unsigned int renderFlags);
void __fastcall ModelRenderScene(CStatus *status);
