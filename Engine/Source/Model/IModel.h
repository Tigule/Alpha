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

  UINT    flags;
  LPCSTR *sequenceNames;
  UINT    numSequences;
  LPCSTR *boneNames;
  UINT    numBones;
  LPCSTR *cameraNames;
  UINT    numCameras;
};

namespace NTempest {
  class CAaBox;
  class CAaSphere;
  class C34Matrix;
  class C3Vector;
  class C44Matrix;
  class CImVector;
}  // namespace NTempest

typedef void (*MODELPROJECT2DCALLBACK)(const NTempest::CAaBox &bounds, NTempest::CImVector color, const NTempest::C44Matrix &view);

DECLARE_DERIVED_HANDLE(HMODEL, HOBJECT);

int ModelGetEventObjectPosition(HMODEL model, UINT objectId, int modelSpace, NTempest::C3Vector *position);

void ModelInitialize();
void ModelDestroy();
void ModelCacheFlush();
void ModelRemoveFromCache(LPCSTR sourcefile);
int  ModelCacheUpdate(DWORD currentTime, CStatus *status);
int  ModelIsLoaded(HMODEL modelHandle, int doLinkedModels);
void ModelProcessEvents(HMODEL model, const NTempest::C34Matrix &orientation);
void ModelProcessEvents(HMODEL model, const NTempest::C3Vector &position, float rotationAngle, const NTempest::C3Vector &rotationAxis, float scale);
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
int    ModelAdvanceTime(HMODEL model);
int    ModelSetSequence(HMODEL model, UINT seqIndex, UINT flags);
int    ModelSetSequence(HMODEL model, UINT seqIndex, UINT objectId, UINT flags);
int    ModelMatchSequence(HMODEL model, UINT objectId, UINT sameAsObjectId, UINT flags);
int    ModelSetRandomSequenceFidget(HMODEL model, UINT seqIndex, UINT flags);
int    ModelSetRandomSequenceFidget(HMODEL model, UINT seqIndex, UINT objectId, UINT flags);
int    ModelSetSequenceFidget(HMODEL model, UINT seqIndex, UINT fidgetId, UINT flags);
int    ModelSetSequenceFidget(HMODEL model, UINT seqIndex, UINT fidgetId, UINT objectId, UINT flags);
UINT   ModelGetNumSequenceFidgets(HMODEL model, UINT seqIndex);
int    ModelGetSequenceDuration(HMODEL model, UINT seqIndex, UINT *duration);
int    ModelGetSequenceTime(HMODEL model, UINT seqIndex);
float  ModelGetPrimarySequenceCompletion(HMODEL model);
int    ModelHasSequenceId(HMODEL model, UINT seqIndex);
HMODEL ModelCreate(LPCSTR sourcefile, CModelCreate *data, CStatus *status);
HMODEL ModelCreateSolidSphere(float radius, HTEXTURE texture);
HMODEL ModelCreateBox(const NTempest::CAaBox &bounds, HTEXTURE texture, EGxBlend blendMode);
HMODEL ModelDuplicate(HMODEL sourceModel, UINT flags);
int    ModelAddLink(HMODEL parent, UINT parentIndex, HMODEL child, float scale);
int    ModelClearLink(HMODEL parent, UINT parentIndex);
int    ModelRemoveLink(HMODEL parent, UINT parentIndex, HMODEL child);
void   ModelClearAllLinks(HMODEL parent);
int    ModelGetLinkPoint(HMODEL model, UINT index, HMODEL *modelList, UINT *entriesInOut);
int    ModelGetNumLinkedAtPoint(HMODEL model, UINT index, UINT *numLinked);
int    ModelHasLinkPoint(HMODEL model, UINT index);
UINT   ModelGetNumLinkPoints(HMODEL model);
int    ModelReplaceTexture(HMODEL model, UINT replaceableId, HTEXTURE texture, int doLinkedModels);
int    ModelGetExtents(HMODEL model, NTempest::CAaBox *extents);
int    ModelGetSeqExtents(HMODEL model, UINT seqnum, NTempest::CAaBox *extents);
int    ModelGetBounds(HMODEL model, NTempest::CAaSphere *bounds);
void   ModelSceneCalcFrustumPlanes();
int    ModelTestSphere(HMODEL model, const NTempest::C34Matrix &orientation, float scale, int testLinkedModels);
int    ModelTestSphere(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    float                     scale,
    int                       testLinkedModels
);
int ModelHitTestSphere(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos);
int ModelHasHitTestVolumes(HMODEL model);
int ModelHitTestVolumes(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos);
int ModelHitTestGeometry(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos);
int ModelGetModelSpacePivot(HMODEL model, UINT objectId, NTempest::C3Vector *pivot);
HCAMERA         ModelGetCamera(HMODEL model, UINT index);
UINT            ModelGetNumCameras(HMODEL model);
int             ModelIsCameraEnabled(HMODEL model, UINT index);
int             ModelIsShowingBoundingSphere(HMODEL model);
int             ModelIsShowingBoundingBox(HMODEL model);
int             ModelIsShowingHitTestGeometry(HMODEL model);
void            ModelRestoreBlendMode(HMODEL model, int doLinkedModels);
void            ModelSetBlendMode(HMODEL model, EGxBlend blendMode, int doLinkedModels);
UINT            ModelGetNumLights(HMODEL model);
const CGxLight *ModelGetLight(HMODEL model, UINT index);
void            ModelEnableFullAlpha(HMODEL model, int enable);
void            ModelEnableAnimBlending(HMODEL model, int enabled);
void            ModelSetEmissiveColor(HMODEL model, const NTempest::CImVector &color, int doLinkedModels);
void            ModelHideBounds(HMODEL model);
void            ModelHideGeosets(HMODEL model, UINT selectionGroup, int hide);
void            ModelHideGeosetsRange(HMODEL model, UINT selectionStart, UINT selectionEnd, int hide);
int             ModelOptimizeVisibleGeosets(HMODEL model);
void            ModelShowCollision(HMODEL model, int show);
void            ModelShowCollisionAaBox(HMODEL model, int show);
void            ModelShowModel(HMODEL model, int show);
void            ModelSetProject2dCallback(MODELPROJECT2DCALLBACK callback);
void            ModelScenePlaceCamera(const NTempest::C3Vector &position, const NTempest::C3Vector &direction);
void            ModelSceneSetSharpness(float sharpness);
void            ModelSetLightSelectCallback(
    HMODEL model,
    void (*callback)(LPVOID, NTempest::C3Vector, const NTempest::C3Vector &, UINT),
    LPVOID parm,
    int    doLinkedModels
);
void ModelSetEventCallback(HMODEL model, void (*callback)(LPCSTR, const NTempest::C3Vector &, LPVOID), LPVOID param, int doLinkedModels);
int  ModelForceSequenceTime(HMODEL model, UINT seqIndex, int timeOffset, int doLinkedModels);
int  ModelForceCurrentSequenceTime(HMODEL model, int timeOffset, int doLinkedModels);
int  ModelGetSequenceMoveSpeed(HMODEL model, UINT seqIndex, float *moveSpeed);
int  ModelGetObjectPosition(HMODEL model, UINT objectId, NTempest::C3Vector *position);
void ModelSetSeqFinishedHandler(HMODEL model, int (*callback)(LPVOID), LPVOID param);
void ModelSetSeqFinishedHandler(HMODEL model, UINT sequence, int (*callback)(LPVOID), LPVOID param);
void ModelSetTimeScale(HMODEL model, float timeScale, int doLinkedModels);
int  ModelSetObjectTimeScale(HMODEL model, UINT objectId, float timeScale, int doLinkedModels);
int  ModelApplyObjectLookAt(HMODEL model, UINT objectId, const NTempest::C3Vector &target);
int  ModelRemoveObjectLookAt(HMODEL model, UINT objectId);
int  ModelApplyObjectFaceDir(HMODEL model, UINT objectId, const NTempest::C3Vector &direction);
int  ModelRemoveObjectFaceDir(HMODEL model, UINT objectId);
int  ModelMarkFootstepSequence(HMODEL model, UINT seqIndex);
int  ModelLockObjectSequence(HMODEL model, UINT objectId, int set);
void ModelSetVertexColor(HMODEL model, BYTE red, BYTE green, BYTE blue, int doLinkedModels);
void ModelGetVertexColor(HMODEL model, BYTE &red, BYTE &green, BYTE &blue);
void ModelShowUnselectable(HMODEL model, BYTE red, BYTE green, BYTE blue);
void ModelHideUnselectable(HMODEL model);
int  ModelIsShowingUnselectable(HMODEL model);
void ModelSetVertexAlpha(HMODEL model, BYTE alpha, int doLinkedModels);
BYTE ModelGetVertexAlpha(HMODEL model);
void ModelCustGeosetMove(HMODEL model, UINT custGeosetId, const NTempest::C3Vector &modelSpacePosition);
void ModelCustGeosetRemove(HMODEL model, UINT custGeosetId);
void ModelCustGeosetAdd(
    HMODEL                    model,
    const NTempest::C3Vector &modelSpacePosition,
    void (*renderCallback)(HMODEL, const NTempest::C34Matrix &, LPVOID),
    LPVOID renderParam,
    UINT  *custGeosetId
);
UINT ModelGetPrimarySequence(HMODEL model);
int  ModelUsesBlending(HMODEL model);
void ModelEnumAnimObjects(HMODEL model, int (*callbackfcn)(UINT, LPCSTR, LPVOID), LPVOID param);
void ModelEnableEmitters(HMODEL model, int enable, int doLinkedModels);
void ModelEnableRibbons(HMODEL model, int enable);
int  ModelAnimHasObjectId(HMODEL model, UINT objectId);
UINT ModelGetNumTextures(HMODEL model);
UINT ModelGetTextureReplaceableId(HMODEL model, UINT textureId);
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
void ModelAddToScene(HMODEL model, UINT renderFlags);
void ModelAddToScene(const NTempest::C3Vector &position, int priorityPlane, void (*callback)(LPVOID, int), LPVOID param1, int param2);
void ModelRender(HMODEL model, CStatus *status, UINT renderFlags);
void ModelRenderScene(CStatus *status);
