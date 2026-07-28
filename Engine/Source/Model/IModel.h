#pragma once

#include "Base/Handle.h"
#include "Gx/Gx.h"
#include "Services/Camera.h"

class CStatus;
class CGxLight;
struct HTEXTURE__;
typedef HTEXTURE__ *HTEXTURE;

struct CModelCreate {
  CModelCreate() {
  }

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

typedef void(*MODELPROJECT2DCALLBACK)(const NTempest::CAaBox &bounds, NTempest::CImVector color, const NTempest::C44Matrix &view);

DECLARE_DERIVED_HANDLE(HMODEL, HOBJECT);

int ModelGetEventObjectPosition(HMODEL model, unsigned int objectId, int modelSpace, NTempest::C3Vector *position);

void ModelInitialize();
void ModelDestroy();
void ModelCacheFlush();
void ModelRemoveFromCache(const char *sourcefile);
int ModelCacheUpdate(unsigned long currentTime, CStatus *status);
int ModelIsLoaded(HMODEL modelHandle, int doLinkedModels);
void ModelProcessEvents(HMODEL model, const NTempest::C34Matrix &orientation);
void
ModelProcessEvents(HMODEL model, const NTempest::C3Vector &position, float rotationAngle, const NTempest::C3Vector &rotationAxis, float scale);
void ModelGetStandingMatrix(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    const NTempest::C3Vector &groundNormal,
    float                     facing,
    float                     scale,
    NTempest::C34Matrix      *orientation
);
void ModelForceStandingMatrix(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    const NTempest::C3Vector &groundNormal,
    float                     facing,
    float                     scale,
    int                       enumGroundTrack,
    float                     blendRatio,
    NTempest::C34Matrix      *orientation
);
int ModelAdvanceTime(HMODEL model);
int ModelSetSequence(HMODEL model, unsigned int seqIndex, unsigned int flags);
int ModelSetSequence(HMODEL model, unsigned int seqIndex, unsigned int objectId, unsigned int flags);
int ModelMatchSequence(HMODEL model, unsigned int objectId, unsigned int sameAsObjectId, unsigned int flags);
int ModelSetRandomSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int flags);
int ModelSetRandomSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int objectId, unsigned int flags);
int ModelSetSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int fidgetId, unsigned int flags);
int ModelSetSequenceFidget(HMODEL model, unsigned int seqIndex, unsigned int fidgetId, unsigned int objectId, unsigned int flags);
unsigned int ModelGetNumSequenceFidgets(HMODEL model, unsigned int seqIndex);
int ModelGetSequenceDuration(HMODEL model, unsigned int seqIndex, unsigned int *duration);
int ModelGetSequenceTime(HMODEL model, unsigned int seqIndex);
float ModelGetPrimarySequenceCompletion(HMODEL model);
int ModelHasSequenceId(HMODEL model, unsigned int seqIndex);
HMODEL ModelCreate(const char *sourcefile, CModelCreate *data, CStatus *status);
HMODEL ModelCreateSolidSphere(float radius, HTEXTURE texture);
HMODEL ModelCreateBox(const NTempest::CAaBox &bounds, HTEXTURE texture, EGxBlend blendMode);
HMODEL ModelDuplicate(HMODEL sourceModel, unsigned int flags);
int ModelAddLink(HMODEL parent, unsigned int parentIndex, HMODEL child, float scale);
int ModelClearLink(HMODEL parent, unsigned int parentIndex);
int ModelRemoveLink(HMODEL parent, unsigned int parentIndex, HMODEL child);
void ModelClearAllLinks(HMODEL parent);
int ModelGetLinkPoint(HMODEL model, unsigned int index, HMODEL *modelList, unsigned int *entriesInOut);
int ModelGetNumLinkedAtPoint(HMODEL model, unsigned int index, unsigned int *numLinked);
int ModelHasLinkPoint(HMODEL model, unsigned int index);
unsigned int ModelGetNumLinkPoints(HMODEL model);
int ModelReplaceTexture(HMODEL model, unsigned int replaceableId, HTEXTURE texture, int doLinkedModels);
int ModelGetExtents(HMODEL model, NTempest::CAaBox *extents);
int ModelGetSeqExtents(HMODEL model, unsigned int seqnum, NTempest::CAaBox *extents);
int ModelGetBounds(HMODEL model, NTempest::CAaSphere *bounds);
void ModelSceneCalcFrustumPlanes();
int ModelTestSphere(HMODEL model, const NTempest::C34Matrix &orientation, float scale, int testLinkedModels);
int ModelTestSphere(
    HMODEL model,
    const NTempest::C3Vector &position,
    float rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    float scale,
    int testLinkedModels
);
int ModelHitTestSphere(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos);
int ModelHasHitTestVolumes(HMODEL model);
int ModelHitTestVolumes(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos);
int ModelHitTestGeometry(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos);
int ModelGetModelSpacePivot(HMODEL model, unsigned int objectId, NTempest::C3Vector *pivot);
HCAMERA ModelGetCamera(HMODEL model, unsigned int index);
unsigned int ModelGetNumCameras(HMODEL model);
int ModelIsCameraEnabled(HMODEL model, unsigned int index);
int ModelIsShowingBoundingSphere(HMODEL model);
int ModelIsShowingBoundingBox(HMODEL model);
int ModelIsShowingHitTestGeometry(HMODEL model);
void ModelRestoreBlendMode(HMODEL model, int doLinkedModels);
void ModelSetBlendMode(HMODEL model, EGxBlend blendMode, int doLinkedModels);
unsigned int ModelGetNumLights(HMODEL model);
const CGxLight *ModelGetLight(HMODEL model, unsigned int index);
void ModelEnableFullAlpha(HMODEL model, int enable);
void ModelEnableAnimBlending(HMODEL model, int enabled);
void ModelSetEmissiveColor(HMODEL model, const NTempest::CImVector &color, int doLinkedModels);
void ModelHideBounds(HMODEL model);
void ModelHideGeosets(HMODEL model, unsigned int selectionGroup, int hide);
void ModelHideGeosetsRange(HMODEL model, unsigned int selectionStart, unsigned int selectionEnd, int hide);
int ModelOptimizeVisibleGeosets(HMODEL model);
void ModelShowCollision(HMODEL model, int show);
void ModelShowCollisionAaBox(HMODEL model, int show);
void ModelShowModel(HMODEL model, int show);
void ModelSetProject2dCallback(MODELPROJECT2DCALLBACK callback);
void ModelScenePlaceCamera(const NTempest::C3Vector &position, const NTempest::C3Vector &direction);
void ModelSceneSetSharpness(float sharpness);
void ModelSetLightSelectCallback(
    HMODEL model,
    void(*callback)(void *, NTempest::C3Vector, const NTempest::C3Vector &, unsigned int),
    void *parm,
    int   doLinkedModels
);
void
ModelSetEventCallback(HMODEL model, void(*callback)(const char *, const NTempest::C3Vector &, void *), void *param, int doLinkedModels);
int ModelForceSequenceTime(HMODEL model, unsigned int seqIndex, int timeOffset, int doLinkedModels);
int ModelForceCurrentSequenceTime(HMODEL model, int timeOffset, int doLinkedModels);
int ModelGetSequenceMoveSpeed(HMODEL model, unsigned int seqIndex, float *moveSpeed);
int ModelGetObjectPosition(HMODEL model, unsigned int objectId, NTempest::C3Vector *position);
void ModelSetSeqFinishedHandler(HMODEL model, int(*callback)(void *), void *param);
void ModelSetSeqFinishedHandler(HMODEL model, unsigned int sequence, int(*callback)(void *), void *param);
void ModelSetTimeScale(HMODEL model, float timeScale, int doLinkedModels);
int ModelSetObjectTimeScale(HMODEL model, unsigned int objectId, float timeScale, int doLinkedModels);
int ModelApplyObjectLookAt(HMODEL model, unsigned int objectId, const NTempest::C3Vector &target);
int ModelRemoveObjectLookAt(HMODEL model, unsigned int objectId);
int ModelApplyObjectFaceDir(HMODEL model, unsigned int objectId, const NTempest::C3Vector &direction);
int ModelRemoveObjectFaceDir(HMODEL model, unsigned int objectId);
int ModelMarkFootstepSequence(HMODEL model, unsigned int seqIndex);
int ModelLockObjectSequence(HMODEL model, unsigned int objectId, int set);
void ModelSetVertexColor(HMODEL model, unsigned char red, unsigned char green, unsigned char blue, int doLinkedModels);
void ModelGetVertexColor(HMODEL model, unsigned char &red, unsigned char &green, unsigned char &blue);
void ModelShowUnselectable(HMODEL model, unsigned char red, unsigned char green, unsigned char blue);
void ModelHideUnselectable(HMODEL model);
int ModelIsShowingUnselectable(HMODEL model);
void ModelSetVertexAlpha(HMODEL model, unsigned char alpha, int doLinkedModels);
unsigned char ModelGetVertexAlpha(HMODEL model);
void ModelCustGeosetMove(HMODEL model, unsigned int custGeosetId, const NTempest::C3Vector &modelSpacePosition);
void ModelCustGeosetRemove(HMODEL model, unsigned int custGeosetId);
void ModelCustGeosetAdd(
    HMODEL model, const NTempest::C3Vector &modelSpacePosition,
    void(*renderCallback)(
        HMODEL, const NTempest::C34Matrix &, void *),
    void *renderParam, unsigned int *custGeosetId);
unsigned int ModelGetPrimarySequence(HMODEL model);
int ModelUsesBlending(HMODEL model);
void ModelEnumAnimObjects(HMODEL model, int(*callbackfcn)(unsigned int, const char *, void *), void *param);
void ModelEnableEmitters(HMODEL model, int enable, int doLinkedModels);
void ModelEnableRibbons(HMODEL model, int enable);
int ModelAnimHasObjectId(HMODEL model, unsigned int objectId);
unsigned int ModelGetNumTextures(HMODEL model);
unsigned int ModelGetTextureReplaceableId(HMODEL model, unsigned int textureId);
void ModelAnimate(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    float                     scale,
    const NTempest::C3Vector &cameraWorldPos,
    const NTempest::C3Vector &cameraVector
);
void ModelAnimateCameras(HMODEL model, const NTempest::C34Matrix &orientation);
void ModelResetGlobalSequenceTimes(HMODEL model, int doLinkedModels);
void ModelAnimate(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    const NTempest::C3Vector &nonUniformScale,
    const NTempest::C3Vector &cameraWorldPos,
    const NTempest::C3Vector &cameraVector
);
void ModelAnimate(
    HMODEL                     model,
    const NTempest::C34Matrix &orientation,
    float                      scale,
    const NTempest::C3Vector  &cameraWorldPos,
    const NTempest::C3Vector  &cameraVector
);
void ModelAddToScene(HMODEL model, unsigned int renderFlags);
void ModelAddToScene(const NTempest::C3Vector &position, int priorityPlane, void(*callback)(void *, int), void *param1, int param2);
void ModelRender(HMODEL model, CStatus *status, unsigned int renderFlags);
void ModelRenderScene(CStatus *status);
