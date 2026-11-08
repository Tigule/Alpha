#pragma once

#include "Base/Handle.h"
#include "Gx/Gx.h"
#include "Services/Camera.h"

class CStatus;
class CGxLight;
struct HTEXTURE__;
typedef HTEXTURE__ *HTEXTURE;

enum {
  MODEL_GEO_UNSHADED = 1,
  MODEL_GEO_TWOSIDED = 16,
  MODEL_GEO_UNFOGGED = 32,
  MODEL_GEO_NO_DEPTH_TEST = 64,
  MODEL_GEO_NO_DEPTH_SET = 128
};

struct CModelCreate {
  CModelCreate() : flags(0), sequenceNames(0), numSequences(0), boneNames(0), numBones(0), cameraNames(0), numCameras(0) {
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

DECLARE_DERIVED_HANDLE(HMODELSHARED, HOBJECT);

enum EModelLoad {
  CMODEL_UNINITIALIZED = 0,
  CMODEL_LOADED = 1,
  CMODEL_ASYNC_WAIT = 2,
  CMODEL_DUPE_WAIT = 3,
  CMODEL_DUPE_WAIT_PRSRV_ANIM = 4
};

class CAsyncObject;
class CModelBase;
enum EModelModQ {
  MODEL_ADD_LINK = 0,
  MODEL_APPLY_OBJECT_FACE_DIR = 1,
  MODEL_APPLY_OBJECT_LOOK_AT = 2,
  MODEL_CLEAR_ALL_LINKS = 3,
  MODEL_CLEAR_LINK = 4,
  MODEL_ENABLE_ANIM_BLENDING = 5,
  MODEL_ENABLE_FULL_ALPHA = 6,
  MODEL_FINISH_DUPLICATION = 7,
  MODEL_FORCE_CURRENT_SEQUENCE_TIME = 8,
  MODEL_FORCE_SEQUENCE_TIME = 9,
  MODEL_HIDE_BOUNDS = 10,
  MODEL_HIDE_GEOSETS = 11,
  MODEL_HIDE_GEOSETS_RANGE = 12,
  MODEL_LOCK_OBJECT_SEQUENCE = 13,
  MODEL_MARK_FOOTSTEP_SEQUENCE = 14,
  MODEL_MATCH_SEQUENCE = 15,
  MODEL_OPTIMIZE_VISIBLE_GEOSETS = 16,
  MODEL_REMOVE_LINK = 17,
  MODEL_REMOVE_OBJECT_FACE_DIR = 18,
  MODEL_REMOVE_OBJECT_LOOK_AT = 19,
  MODEL_REPLACE_TEXTURE = 20,
  MODEL_SET_EMISSIVE_COLOR = 21,
  MODEL_SET_EVENT_CALLBACK = 22,
  MODEL_SET_LIGHT_SELECT_CALLBACK = 23,
  MODEL_SET_OBJECT_TIME_SCALE = 24,
  MODEL_SET_RANDOM_SEQUENCE_FIDGET1 = 25,
  MODEL_SET_RANDOM_SEQUENCE_FIDGET2 = 26,
  MODEL_SET_SEQ_FINISHED_HANDLER1 = 27,
  MODEL_SET_SEQ_FINISHED_HANDLER2 = 28,
  MODEL_SET_SEQUENCE1 = 29,
  MODEL_SET_SEQUENCE2 = 30,
  MODEL_SET_SEQUENCE_FIDGET1 = 31,
  MODEL_SET_SEQUENCE_FIDGET2 = 32,
  MODEL_SET_TIME_SCALE = 33,
  MODEL_SET_VERTEX_ALPHA = 34,
  MODEL_SET_VERTEX_COLOR = 35,
  MODEL_SHOW_BOUNDING_SPHERE = 36,
  MODEL_SHOW_COLLISION = 37,
  MODEL_SHOW_COLLISION_AABOX = 38,
  MODEL_SHOW_MODEL = 39,
  MODEL_NUM_COMMANDS = 40,
  MODEL_COMMAND_NOT_QUEUED = MODEL_NUM_COMMANDS
};

NODEDECL(CModelModItem) {
  EModelModQ action;
  BYTE       paramData[16];
};


class CModel : public CHandleObject {
 public:
  CModel(EModelLoad state = CMODEL_ASYNC_WAIT) : asyncObject(0), createData(0), shared(0), state(state) {
  }
  CModel(CModel &source);
  void FinishDuplication(CModel &source);
  void DeleteAsyncObj();
  virtual ~CModel();

 private:
  CModel &operator=(const CModel &source);
  void    RemoveModelCommandsFromQueue();

 public:

  union {
    CAsyncObject *asyncObject;
    CModelBase   *data;
    HMODEL        dupSource;
  };
  CModelCreate *createData;
  HMODELSHARED  shared;
  EModelLoad    state;
  LISTDECL(CModelModItem, modelModQueue);

};

BOOL ModelGetEventObjectPosition(HMODEL model, UINT objectId, int modelSpace, NTempest::C3Vector *position);

void ModelInitialize();
void ModelDestroy();
void ModelCacheFlush();
void ModelRemoveFromCache(LPCSTR sourcefile);
BOOL ModelCacheUpdate(DWORD currentTime, CStatus *status);
BOOL ModelIsLoaded(HMODEL modelHandle, int doLinkedModels = 1);
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
BOOL   ModelAdvanceTime(HMODEL model);
BOOL   ModelSetSequence(HMODEL model, UINT seqIndex, UINT flags);
BOOL   ModelSetSequence(HMODEL model, UINT seqIndex, UINT objectId, UINT flags);
int    ModelMatchSequence(HMODEL model, UINT objectId, UINT sameAsObjectId, UINT flags);
int    ModelSetRandomSequenceFidget(HMODEL model, UINT seqIndex, UINT flags);
int    ModelSetRandomSequenceFidget(HMODEL model, UINT seqIndex, UINT objectId, UINT flags);
int    ModelSetSequenceFidget(HMODEL model, UINT seqIndex, UINT fidgetId, UINT flags);
int    ModelSetSequenceFidget(HMODEL model, UINT seqIndex, UINT fidgetId, UINT objectId, UINT flags);
UINT   ModelGetNumSequenceFidgets(HMODEL model, UINT seqIndex);
BOOL   ModelGetSequenceDuration(HMODEL model, UINT seqIndex, UINT *duration);
int    ModelGetSequenceTime(HMODEL model, UINT seqIndex);
float  ModelGetPrimarySequenceCompletion(HMODEL model);
BOOL   ModelHasSequenceId(HMODEL model, UINT seqIndex);
HMODEL ModelCreate(LPCSTR sourcefile, CModelCreate *data, CStatus *status);
HMODEL ModelCreateSolidSphere(float radius, HTEXTURE texture);
HMODEL ModelCreateBox(const NTempest::CAaBox &bounds, HTEXTURE texture, EGxBlend blendMode);
HMODEL ModelDuplicate(HMODEL sourceModel, UINT flags);
BOOL   ModelAddLink(HMODEL parent, UINT parentIndex, HMODEL child, float scale);
BOOL   ModelClearLink(HMODEL parent, UINT parentIndex);
BOOL   ModelRemoveLink(HMODEL parent, UINT parentIndex, HMODEL child);
void   ModelClearAllLinks(HMODEL parent);
BOOL   ModelGetLinkPoint(HMODEL model, UINT index, HMODEL *modelList, UINT *entriesInOut);
BOOL   ModelGetNumLinkedAtPoint(HMODEL model, UINT index, UINT *numLinked);
BOOL   ModelHasLinkPoint(HMODEL model, UINT index);
UINT   ModelGetNumLinkPoints(HMODEL model);
BOOL   ModelReplaceTexture(HMODEL model, UINT replaceableId, HTEXTURE texture, int doLinkedModels);
BOOL   ModelGetExtents(HMODEL model, NTempest::CAaBox *extents);
BOOL   ModelGetSeqExtents(HMODEL model, UINT seqnum, NTempest::CAaBox *extents);
BOOL   ModelGetBounds(HMODEL model, NTempest::CAaSphere *bounds);
void   ModelSceneCalcFrustumPlanes();
BOOL   ModelTestSphere(HMODEL model, const NTempest::C34Matrix &orientation, float scale, int testLinkedModels);
BOOL   ModelTestSphere(
    HMODEL                    model,
    const NTempest::C3Vector &position,
    float                     rotationAngle,
    const NTempest::C3Vector &rotationAxis,
    float                     scale,
    int                       testLinkedModels
);
BOOL ModelHitTestSphere(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos);
BOOL ModelHasHitTestVolumes(HMODEL model);
BOOL ModelHitTestVolumes(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos);
BOOL ModelHitTestGeometry(HMODEL model, float scale, const NTempest::C3Vector &a, const NTempest::C3Vector &b, int testLinkedModels, float *linePos);
BOOL ModelGetModelSpacePivot(HMODEL model, UINT objectId, NTempest::C3Vector *pivot);
HCAMERA         ModelGetCamera(HMODEL model, UINT index);
UINT            ModelGetNumCameras(HMODEL model);
int             ModelIsCameraEnabled(HMODEL model, UINT index);
BOOL            ModelIsShowingBoundingSphere(HMODEL model);
BOOL            ModelIsShowingBoundingBox(HMODEL model);
BOOL            ModelIsShowingHitTestGeometry(HMODEL model);
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
BOOL            ModelOptimizeVisibleGeosets(HMODEL model);
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
BOOL ModelForceSequenceTime(HMODEL model, UINT seqIndex, int timeOffset, int doLinkedModels);
BOOL ModelForceCurrentSequenceTime(HMODEL model, int timeOffset, int doLinkedModels);
BOOL ModelGetSequenceMoveSpeed(HMODEL model, UINT seqIndex, float *moveSpeed);
BOOL ModelGetObjectPosition(HMODEL model, UINT objectId, NTempest::C3Vector *position);
void ModelSetSeqFinishedHandler(HMODEL model, int (*callback)(LPVOID), LPVOID param);
void ModelSetSeqFinishedHandler(HMODEL model, UINT sequence, int (*callback)(LPVOID), LPVOID param);
void ModelSetTimeScale(HMODEL model, float timeScale, int doLinkedModels);
BOOL ModelSetObjectTimeScale(HMODEL model, UINT objectId, float timeScale, int doLinkedModels);
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
BOOL ModelIsShowingUnselectable(HMODEL model);
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
BOOL ModelUsesBlending(HMODEL model);
void ModelEnumAnimObjects(HMODEL model, int (*callbackfcn)(UINT, LPCSTR, LPVOID), LPVOID param);
void ModelEnableEmitters(HMODEL model, int enable, int doLinkedModels);
void ModelEnableRibbons(HMODEL model, int enable);
BOOL ModelAnimHasObjectId(HMODEL model, UINT objectId);
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
