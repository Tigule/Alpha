#pragma once

#include "Anim/AnimTypes.h"
#include "Base/Color.h"
#include "Anim/Interp.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/c4quaternioncompressed.h"
#include "Tempest/cmath.h"

#include "Tempest/caabox.h"
#include "Tempest/c3vector.h"

#include <stddef.h>
#include <stpl.h>

struct CAnimCameraObjStatus;
struct CAnimEmitter2ObjStatus;
struct CAnimEventObjStatus;
struct CAnimLayerStatus;
struct CAnimLightObjStatus;
struct CAnimObjBlendStatus;
struct CAnimObj;
struct CAnimRibbonObjStatus;
struct CAnimVisibleObj;
struct CAnimBoneObj;
struct CAnimCameraObj;
struct CAnimEmitter2Obj;
struct CAnimEventObj;
struct CAnimLightObj;
struct CAnimMaterialLayer;
struct CAnimRibbonObj;
struct CAnimData;
struct CAnimGeoset;
struct AnimInfo;
struct InterpInfo;
class C3Color;
template <class T, class U>
class CKeyFrameTrack;
namespace NTempest {
  class CImVector;
  class C4Quaternion;
}  // namespace NTempest

static void SetGeosetColor(
    const InterpInfo &animInfo,
    CAnimGeoset *currgeoset,
    CAnimGeosetObjStatus *geoStatus,
    NTempest::CImVector *currentColor
);
static void SetGeosetAlpha(
    const InterpInfo &animInfo, CAnimGeoset *currgeoset, CAnimGeosetObjStatus *geoStatus, CGeosetColor *color
);
static void AnimateAllMaterialLayers(AnimInfo *animInfo, unsigned int *tex);

#ifndef MDL_TRACK_TYPE_DEFINED
#define MDL_TRACK_TYPE_DEFINED
enum MDLTRACKTYPE {
  TRACK_DONT_INTERP = 0,
  TRACK_LINEAR = 1,
  TRACK_HERMITE = 2,
  TRACK_BEZIER = 3,
  NUM_TRACK_TYPES = 4
};
#endif

enum KEYTYPE {
  KEY_DONT_INTERP = 0,
  KEY_LINEAR = 1,
  KEY_HERMITE = 2,
  KEY_BEZIER = 3
};

enum OBJECTTYPE {
  OBJ_TYPE_HELPER = 0,
  OBJ_TYPE_LIGHT = 1,
  OBJ_TYPE_MODEL = 2,
  OBJ_TYPE_BONE = 3,
  OBJ_TYPE_EMITTER2 = 4,
  OBJ_TYPE_RIBBON = 5,
  OBJ_TYPE_EVENT = 6,
  NUM_OBJ_TYPES = 7
};

void AnimObjectSetIndex(CAnimData *shared, CAnimObj *objptr, unsigned int index);
CAnimObj *GetNodeByIndex(CAnimData *shared, unsigned int nodeIndex);
int AnimObjectSetParent(CAnimData *shared, CAnimObj *objptr, unsigned int parentIndex);

unsigned char *AnimObjectSetEventTrack(unsigned char *data, unsigned int bytesLeft, CAnimData *shared, CAnimEventObj *objptr);
unsigned char *AnimObjectSetRibbonSlot(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimRibbonObj *objptr);

unsigned char *AddKeyFramesType(
    unsigned char                *data,
    unsigned int                  bytesRemaining,
    unsigned long                 tag,
    CAnimData                    *shared,
    CKeyFrameTrack<float, float> *track,
    MDLTRACKTYPE                  forceType
);
unsigned char *
AnimObjectSetRotation(unsigned char *data, unsigned int bytesRemaining, CAnimData *shared, CAnimObj *objptr, MDLTRACKTYPE forceType);
unsigned char *
AnimObjectSetAttenuation(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType);
unsigned char *
AnimObjectSetColor(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType);
unsigned char *
AnimObjectSetIntensity(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType);
unsigned char *
AnimObjectSetAmbColor(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType);
unsigned char *
AnimObjectSetAmbIntensity(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType);
unsigned char *
AnimObjectSetVisibilityTrack(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimVisibleObj *objptr, MDLTRACKTYPE forceType);
unsigned char *
AnimObjectSetRibbonHeightAbove(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType);
unsigned char *
AnimObjectSetRibbonHeightBelow(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType);
unsigned char *
AnimObjectSetRibbonColor(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType);
unsigned char *
AnimObjectSetRibbonAlpha(unsigned char *data, unsigned int fileBytes, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType);
unsigned char *AddKeyFramesType(
    unsigned char                                          *data,
    unsigned int                                            bytesRemaining,
    unsigned long                                           tag,
    CAnimData                                              *shared,
    CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector> *track,
    MDLTRACKTYPE                                            forceType
);
struct CKeyFrame {
  int time;
};

template <class T>
struct CLinearKeyFrame : public CKeyFrame {
  T transform;
};

template <class T>
struct CSplineKeyFrame : public CLinearKeyFrame<T> {
  T inTan;
  T outTan;
};

struct CKeySeq {
  unsigned int start;
  unsigned int count;
};

namespace NTempest {
#ifndef MDL_CIRANGE_DEFINED
#define MDL_CIRANGE_DEFINED
  class CiRange {
   public:
    long l;
    long h;
  };
#endif
}  // namespace NTempest

#ifndef MDL_COMMON_TYPES_DEFINED
#define MDL_COMMON_TYPES_DEFINED

template <unsigned int Size>
class CMdlString {
 public:
  operator char *() {
    return m_string;
  }

  operator const char *() const {
    return m_string;
  }

  char &operator[](unsigned int index) {
    return m_string[index];
  }

  char operator[](unsigned int index) const {
    return m_string[index];
  }

 private:
  char m_string[Size];
};

struct CMdlBounds {
  NTempest::CAaBox extent;
  float            radius;
};

#endif

struct CAnimSequence {
  CMdlString<80>    name;
  NTempest::CiRange time;
  float             moveSpeed;
  unsigned int      flags;
  unsigned int      randPickChance;
  NTempest::CiRange replay;
  CMdlBounds        bounds;
  unsigned int      blendTime;
};

struct CVariations {
  CVariations() : primary(0xFF) {
  }

  CArray<unsigned char> variation;
  unsigned int          primary;
};

struct CSeqOrdering {
  CArray<CVariations> order;
  const char        **nameListUsed;
};

class CKeyFrameTrackBase {
  friend void SetGeosetColor(
      const InterpInfo &, CAnimGeoset *, CAnimGeosetObjStatus *, NTempest::CImVector *
  );
  friend void SetGeosetAlpha(const InterpInfo &, CAnimGeoset *, CAnimGeosetObjStatus *, CGeosetColor *);
  friend void AnimateAllMaterialLayers(AnimInfo *, unsigned int *);

 public:
  CKeyFrameTrackBase() : m_keyFrames(0), m_numKeyFrames(0), m_indices(), m_globalSeqId(static_cast<unsigned int>(-1)) {
  }
  ~CKeyFrameTrackBase() {
    if (m_keyFrames)
      SMemFree(m_keyFrames, __FILE__, __LINE__, 0);
  }

  CKeyFrame      *m_keyFrames;
  unsigned int    m_numKeyFrames;
  unsigned int    m_keyFrameSize;
  CArray<CKeySeq> m_indices;
  unsigned int    m_globalSeqId;

  unsigned int TotalKeys() const {
    return m_numKeyFrames;
  }

  void SetGlobalSequenceId(unsigned int globalSeqId) {
    m_globalSeqId = globalSeqId;
  }

  unsigned int NumKeysThisSeq(unsigned int sequence) const {
    ASSERT(SequenceChanges());
    ASSERT(sequence < m_indices.Count());
    return m_indices[sequence].count;
  }

  unsigned int NumKeysThisSeqSafe(unsigned int sequence) {
    if (SequenceNeverChanges()) {
      return TotalKeys();
    }
    if (sequence >= m_indices.Count()) {
      return 0;
    }
    return m_indices[sequence].count;
  }

  int SequenceChanges() const {
    return m_globalSeqId == -1;
  }

  int SequenceNeverChanges() const {
    return m_globalSeqId != -1;
  }

  unsigned int Bytes() const {
    return m_numKeyFrames * m_keyFrameSize;
  }

  unsigned int FirstKeyId(unsigned int sequence) const {
    ASSERT(sequence < m_indices.Count());
    return m_indices[sequence].start;
  }

  unsigned int NextKeyId(unsigned int keyId, unsigned int sequence) const {
    if (SequenceNeverChanges()) {
      return keyId == TotalKeys() - 1 ? 0 : keyId + 1;
    }
    return keyId == LastKeyId(sequence) ? FirstKeyId(sequence) : keyId + 1;
  }

  CKeyFrame   *NextKey(CKeyFrame *key);
  unsigned int SetAnimTime(const CBaseStatus &sequence, CKeyTrackStatus *keyStat, const InterpInfo &interpData);
  void         SetNumKeys(unsigned int numKeys, unsigned int keySize);
  void         AddKey(int time);
  void         SetSequenceIndices(const CArray<CAnimSequence> &seq);
  int          JustPastKey(
               int elapsedTime,
               const CAnimSequence &seqShared,
               int seqElapsed,
               unsigned char sequenceId,
               int seqIsNew,
               const CKeyTrackStatus &prev,
               const CKeyTrackStatus &curr
             ) const;

 protected:
  unsigned int LastKeyId(unsigned int sequence) const {
    ASSERT(sequence < m_indices.Count());
    return m_indices[sequence].start + m_indices[sequence].count - 1;
  }

  const CKeyFrame *NextKey(const CKeyFrame *key) const;
  const CKeyFrame *GetKeyFrame(unsigned int keyId) const;
  CKeyFrame       *GetKeyFrame(unsigned int keyId);
  unsigned int     TimeDiff(const CKeyFrame &curr, const CKeyFrame &next, unsigned int seqTime);
  unsigned int     KeyFrameSize() const {
    return m_keyFrameSize;
  }
  int              JustPastKeyForward(
                     int elapsedTime,
                     const CAnimSequence &seqShared,
                     int seqElapsed,
                     int seqIsNew,
                     const CKeyTrackStatus &prev,
                     const CKeyTrackStatus &curr
                   ) const;
  int              JustPastKeyBackward(
                     int elapsedTime,
                     const CAnimSequence &seqShared,
                     int seqElapsed,
                     int seqIsNew,
                     const CKeyTrackStatus &prev,
                     const CKeyTrackStatus &curr
                   ) const;

 private:
  void         ISetAnimTime(unsigned char sequenceId, int seqIsNew, int milliseconds, int endtime, CKeyTrackStatus *keyStat);
  void         ISetAnimTimeConstSeq(int milliseconds, int endtime, CKeyTrackStatus *keyStat);
  unsigned int FindKeyForTime(unsigned int currSeq, unsigned int currKeyId, int targettime);
  unsigned int FindKeyForTimeConstSeq(unsigned int currKeyId, int targettime);
};

template <class T, class U>
class CKeyFrameTrack : public CKeyFrameTrackBase {
  friend void SetGeosetColor(
      const InterpInfo &, CAnimGeoset *, CAnimGeosetObjStatus *, NTempest::CImVector *
  );
  friend void SetGeosetAlpha(const InterpInfo &, CAnimGeoset *, CAnimGeosetObjStatus *, CGeosetColor *);
  friend void AnimateAllMaterialLayers(AnimInfo *, unsigned int *);

 public:
  CKeyFrameTrack() : m_trackType(KEY_LINEAR) {
  }

  void SetTrackType(KEYTYPE trackType) {
    m_trackType = trackType;
  }

  using CKeyFrameTrackBase::SetNumKeys;

  void SetNumKeys(unsigned int numKeys) {
    switch (m_trackType) {
      case KEY_DONT_INTERP:
      case KEY_LINEAR:
        CKeyFrameTrackBase::SetNumKeys(numKeys, sizeof(CLinearKeyFrame<T>));
        break;
      case KEY_HERMITE:
      case KEY_BEZIER:
        CKeyFrameTrackBase::SetNumKeys(numKeys, sizeof(CSplineKeyFrame<T>));
        break;
    }
  }

  void AddKey(int time, const U &keyData) {
    CKeyFrameTrackBase::AddKey(time);
    GetLinearKey(m_numKeyFrames - 1)->transform = keyData;
  }

  void AddKey(int time, const U &keyData, const U &inTan, const U &outTan) {
    CKeyFrameTrackBase::AddKey(time);
    CSplineKeyFrame<T> *key = GetSplineKey(m_numKeyFrames - 1);
    key->transform = keyData;
    key->inTan = inTan;
    key->outTan = outTan;
  }

  int InterpolateVolatile(const InterpInfo &info, const CBaseStatus &base, CKeyTrackStatus *keyStatus, const U &fallback, U *transform);
  int InterpolateRetained(const InterpInfo &info, const CBaseStatus &base, CKeyTrackStatus *keyStatus, const U &fallback, U *transform);

  unsigned int Bytes() {
    return CKeyFrameTrackBase::Bytes();
  }

  KEYTYPE GetTrackType() {
    return m_trackType;
  }

  CLinearKeyFrame<T> *GetLinearKey(unsigned int index) {
    ASSERT(KeyFrameSize() == sizeof(CLinearKeyFrame<T>));
    return reinterpret_cast<CLinearKeyFrame<T> *>(GetKeyFrame(index));
  }

  CSplineKeyFrame<T> *GetSplineKey(unsigned int index) {
    ASSERT(KeyFrameSize() == sizeof(CSplineKeyFrame<T>));
    return reinterpret_cast<CSplineKeyFrame<T> *>(GetKeyFrame(index));
  }

  const CLinearKeyFrame<T> *ToLinearKey(const CKeyFrame *key) const {
    ASSERT(KeyFrameSize() == sizeof(CLinearKeyFrame<T>));
    return reinterpret_cast<const CLinearKeyFrame<T> *>(key);
  }

  CLinearKeyFrame<T> *ToLinearKey(CKeyFrame *key) {
    ASSERT(KeyFrameSize() == sizeof(CLinearKeyFrame<T>));
    return reinterpret_cast<CLinearKeyFrame<T> *>(key);
  }

  const CSplineKeyFrame<T> *ToSplineKey(const CKeyFrame *key) const {
    ASSERT(KeyFrameSize() == sizeof(CSplineKeyFrame<T>));
    return reinterpret_cast<const CSplineKeyFrame<T> *>(key);
  }

  CSplineKeyFrame<T> *ToSplineKey(CKeyFrame *key) {
    ASSERT(KeyFrameSize() == sizeof(CSplineKeyFrame<T>));
    return reinterpret_cast<CSplineKeyFrame<T> *>(key);
  }

 private:
  int  InterpolateVolatileFewKeys(const CKeyTrackStatus &keyStatus, U *transform);
  int  InterpolateRetainedFewKeys(const CKeyTrackStatus &keyStatus, U *transform);
  void Interpolate(const CKeyTrackStatus &keyStat, unsigned int seqTime, U *transform);
  void InterpolateHermite(const CSplineKeyFrame<T> &currkey, const CSplineKeyFrame<T> &nextkey, float ratio, U *transform);
  void InterpolateBezier(const CSplineKeyFrame<T> &currkey, const CSplineKeyFrame<T> &nextkey, float ratio, U *transform);
  void InterpolateLinear(const CLinearKeyFrame<T> &currkey, const CLinearKeyFrame<T> &nextkey, float ratio, U *transform);

 private:
  KEYTYPE m_trackType;
};

template <>
inline int CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion>::InterpolateVolatileFewKeys(
    const CKeyTrackStatus  &keyStatus,
    NTempest::C4Quaternion *transform
) {
  ASSERT(transform);
  *transform = reinterpret_cast<const CLinearKeyFrame<NTempest::C4QuaternionCompressed> *>(GetKeyFrame(keyStatus.currKey))->transform;
  return 1;
}
template <>
void CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion>::Interpolate(
    const CKeyTrackStatus  &keyStat,
    unsigned int            seqTime,
    NTempest::C4Quaternion *transform
);
template <>
void CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion>::InterpolateHermite(
    const CSplineKeyFrame<NTempest::C4QuaternionCompressed> &currkey,
    const CSplineKeyFrame<NTempest::C4QuaternionCompressed> &nextkey,
    float                                                    ratio,
    NTempest::C4Quaternion                                  *transform
);
template <>
void CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion>::InterpolateBezier(
    const CSplineKeyFrame<NTempest::C4QuaternionCompressed> &currKey,
    const CSplineKeyFrame<NTempest::C4QuaternionCompressed> &nextKey,
    float                                                    ratio,
    NTempest::C4Quaternion                                  *transform
);
template <>
void CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion>::InterpolateLinear(
    const CLinearKeyFrame<NTempest::C4QuaternionCompressed> &currkey,
    const CLinearKeyFrame<NTempest::C4QuaternionCompressed> &nextkey,
    float                                                    ratio,
    NTempest::C4Quaternion                                  *transform
);

template <>
void CKeyFrameTrack<C3Color, C3Color>::Interpolate(
    const CKeyTrackStatus &keyStat, unsigned int seqTime, C3Color *transform
);

struct CAnimTransform {
  int Animates();

  CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector>                   translation;
  CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion> rotation;
  CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector>                   scale;
};

struct CAnimObj : public CAnimTransform {
  CAnimObj(OBJECTTYPE objectType = OBJ_TYPE_HELPER) : animObjId(0), splitIndex(0), type(objectType), flags(0) {
    name[0] = 0;
  }

  unsigned int                animObjId;
  unsigned int                splitIndex;
  char                        name[80];
  TSGrowableArray<CAnimObj *> childarray;
  unsigned int                type : 8;
  unsigned int                flags : 8;
};

struct CAnimBoneObj : public CAnimObj {
  CAnimBoneObj() : CAnimObj(OBJ_TYPE_BONE), geosetId(0) {
  }

  unsigned int geosetId : 8;
};

struct CAnimVisibleObj {
  CKeyFrameTrack<float, float> visibility;
};

struct CAnimCameraObj : public CAnimVisibleObj {
  char                                                   name[80];
  NTempest::C3Vector                                     pivot;
  CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector> translation;
  CKeyFrameTrack<float, float>                           roll;
  NTempest::C3Vector                                     targetPivot;
  CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector> targetTranslation;
};

struct CAnimGeoset : public CAnimVisibleObj {
  CKeyFrameTrack<C3Color, C3Color> color;
  unsigned int                     sgGeosetId;
};

struct CAnimModelObj : public CAnimObj, public CAnimVisibleObj {
  CAnimModelObj() : CAnimObj(OBJ_TYPE_MODEL), geosetId(0xFF) {
  }

  unsigned int geosetId : 8;
};

struct CAnimEventObj : public CAnimObj {
  CAnimEventObj() : CAnimObj(OBJ_TYPE_EVENT) {
  }

  CKeyFrameTrackBase events;
};

struct CAnimRibbonObj : public CAnimObj, public CAnimVisibleObj {
  CAnimRibbonObj() : CAnimObj(OBJ_TYPE_RIBBON) {
  }

  CKeyFrameTrack<float, float>               heightAbove;
  CKeyFrameTrack<float, float>               heightBelow;
  CKeyFrameTrack<C3Color, C3Color>           color;
  CKeyFrameTrack<float, float>               alpha;
  CKeyFrameTrack<unsigned int, unsigned int> slot;
};

struct CAnimMaterialLayer : public CAnimVisibleObj {
  CAnimMaterialLayer() : layerId(0) {
  }

  CKeyFrameTrack<unsigned int, unsigned int> flip;
  unsigned int                               layerId;
};

struct CAnimEmitter2Obj : public CAnimObj, public CAnimVisibleObj {
  CAnimEmitter2Obj() : CAnimObj(OBJ_TYPE_EMITTER2), squirts(0) {
  }

  CKeyFrameTrack<float, float> particleSpeed;
  CKeyFrameTrack<float, float> emissionRate;
  CKeyFrameTrack<float, float> gravity;
  CKeyFrameTrack<float, float> variation;
  CKeyFrameTrack<float, float> latitude;
  CKeyFrameTrack<float, float> longitude;
  CKeyFrameTrack<float, float> length;
  CKeyFrameTrack<float, float> width;
  CKeyFrameTrack<float, float> zsource;
  CKeyFrameTrack<float, float> lifeSpan;
  unsigned int                 squirts;
};

struct CAnimLightObj : public CAnimObj, public CAnimVisibleObj {
  CAnimLightObj() : CAnimObj(OBJ_TYPE_LIGHT) {
  }

  CKeyFrameTrack<float, float>     attenstart;
  CKeyFrameTrack<float, float>     attenend;
  CKeyFrameTrack<C3Color, C3Color> color;
  CKeyFrameTrack<float, float>     intensity;
  CKeyFrameTrack<C3Color, C3Color> ambColor;
  CKeyFrameTrack<float, float>     ambIntensity;
};

template <class T>
struct CCallbackFcn {
  T     callback;
  void *param;
};

struct CSeqInfo {
  CSeqInfo() {
    memset(this, 0, sizeof(*this));
    seqTimeScale = 1.0f;
  }

  int                                     elapsed;
  unsigned int                            useCount : 16;
  unsigned int                            replayTimes : 15;
  unsigned int                            seqFinished : 1;
  CCallbackFcn<int(*)(void *)> finished;
  float                                   seqTimeScale;
  int                                     scaledElapsedTime;
};

struct CAnim : public CHandleObject {
 public:
  CAnim(unsigned int createFlags = 0) : anySeqFinished(), appEvent(), hdata(0), seqLastTime(0), flags(createFlags), primarySeq(0), seqMapIndex(0) {
    anySeqFinished.callback = 0;
    anySeqFinished.param = 0;
    appEvent.callback = 0;
    appEvent.param = 0;
  }
  ~CAnim() {
    if (hdata) {
      HandleClose(hdata);
    }
  }
  CArray<CSeqInfo>                                                                   seq;
  CArray<CAnimObjStatus *>                                                           status;
  CArray<CAnimObjStatus>                                                             baseStatus;
  CArray<CAnimObjStatus>                                                             boneStatus;
  CArray<CAnimGeosetObjStatus>                                                       geosetStatus;
  CArray<CAnimModelObjStatus>                                                        modelStatus;
  CArray<unsigned int>                                                               globalSeqElapsed;
  CArray<CAnimObjBlendStatus>                                                        blendStatus;
  CArray<CAnimLightObjStatus>                                                        lightStatus;
  CArray<CAnimObjStatus>                                                             textureStatus;
  CArray<CAnimEmitter2ObjStatus>                                                     emitter2Status;
  CArray<CAnimRibbonObjStatus>                                                       ribbonStatus;
  CArray<CAnimCameraObjStatus>                                                       cameraStatus;
  CArray<CAnimEventObjStatus>                                                        eventStatus;
  CArray<CAnimLayerStatus>                                                           layerStatus;
  TSGrowableArray<NTempest::C3Vector>                                                lookAtTarget;
  CCallbackFcn<ANIMSEQFINISHEDHANDLER>                                               anySeqFinished;
  CCallbackFcn<void(*)(const char *, const NTempest::C3Vector &, void *)> appEvent;
  HANIMDATA                                                                          hdata;
  unsigned long                                                                      seqLastTime;
  unsigned int                                                                       flags : 8;
  unsigned int                                                                       primarySeq : 8;
  unsigned int                                                                       seqMapIndex : 8;
};

struct CAnimData : public CHandleObject {
  CAnimData() : flags(0) {
  }
  int Animates();
  int Moves();

  TSGrowableArray<CSeqOrdering> seqOrder;
  CArray<unsigned int>          objectOrder;
  CArray<CAnimSequence>         seq;
  CArray<unsigned int>          globalSeqLength;
  CArray<CAnimObj *>            obj;
  CArray<CAnimGeoset>           geo;
  CArray<CAnimTransform>        tex;
  CArray<CAnimObj>              baseObjs;
  CArray<CAnimBoneObj>          boneObjs;
  CArray<CAnimLightObj>         lightObjs;
  CArray<CAnimModelObj>         modelObjs;
  CArray<CAnimEmitter2Obj>      emitter2Objs;
  CArray<CAnimRibbonObj>        ribbonObjs;
  CArray<CAnimCameraObj>        cameraObjs;
  CArray<CAnimEventObj>         eventObjs;
  CArray<CAnimMaterialLayer>    layers;
  TSGrowableArray<unsigned int> geoIdToGeoAnimId;
  TSGrowableArray<CAnimObj *>   headarray;
  unsigned char                 flags;
};

struct InterpInfo {
  InterpInfo(CAnim *container, CAnimData *animptr, const TSFixedArray<NTempest::C3Vector> &positions);

  CAnim                                  *unique;
  CAnimData                              *shared;
  NTempest::C3Vector                      basisX;
  NTempest::C3Vector                      basisY;
  NTempest::C3Vector                      basisZ;
  NTempest::C3Vector                      basisScale;
  NTempest::C3Vector                      basisPosition;
  const TSFixedArray<NTempest::C3Vector> &positions;
};

struct AnimInfo : public InterpInfo {
  AnimInfo(CAnim *container, CAnimData *animptr, const TSFixedArray<NTempest::C3Vector> &positions, const CAnimationData &animationData)
      : InterpInfo(container, animptr, positions), data(animationData) {
  }

  const CAnimationData &data;
  NTempest::C3Vector    cameraVector;
  NTempest::C3Vector    cameraWorldPos;
};

template <class T, class U>
inline int CKeyFrameTrack<T, U>::InterpolateVolatile(
    const InterpInfo  &info,
    const CBaseStatus &base,
    CKeyTrackStatus   *keyStatus,
    const U           &fallback,
    U                 *transform
) {
  if (!NumKeysThisSeqSafe(base.currSeq)) {
    *transform = fallback;
    return 0;
  }
  unsigned int keys = SetAnimTime(base, keyStatus, info);
  if (!keys) {
    *transform = fallback;
    return 0;
  }
  if (keys == 1 || keyStatus->currKey == keyStatus->nextKey) {
    return InterpolateVolatileFewKeys(*keyStatus, transform);
  }
  unsigned int sequenceTime = info.shared->seq[base.currSeq].time.h - info.shared->seq[base.currSeq].time.l;
  Interpolate(*keyStatus, sequenceTime, transform);
  return 1;
}

template <class T, class U>
inline int CKeyFrameTrack<T, U>::InterpolateRetained(
    const InterpInfo  &info,
    const CBaseStatus &base,
    CKeyTrackStatus   *keyStatus,
    const U           &fallback,
    U                 *transform
) {
  if (!NumKeysThisSeqSafe(base.currSeq)) {
    *transform = fallback;
    return 0;
  }
  unsigned int keys = SetAnimTime(base, keyStatus, info);
  if (!keys) {
    *transform = fallback;
    return 0;
  }
  if (keys == 1 || keyStatus->currKey == keyStatus->nextKey) {
    return InterpolateRetainedFewKeys(*keyStatus, transform);
  }
  unsigned int sequenceTime = info.shared->seq[base.currSeq].time.h - info.shared->seq[base.currSeq].time.l;
  Interpolate(*keyStatus, sequenceTime, transform);
  return 1;
}

template <class T, class U>
inline int CKeyFrameTrack<T, U>::InterpolateVolatileFewKeys(const CKeyTrackStatus &keyStatus, U *transform) {
  const unsigned char *key = reinterpret_cast<const unsigned char *>(GetKeyFrame(keyStatus.currKey));
  const unsigned int   valueOffset = sizeof(T) == sizeof(__int64) ? 8 : sizeof(int);
  *transform = *reinterpret_cast<T *>(const_cast<unsigned char *>(key + valueOffset));
  return 1;
}

template <class T, class U>
inline int CKeyFrameTrack<T, U>::InterpolateRetainedFewKeys(const CKeyTrackStatus &keyStatus, U *transform) {
  const unsigned char *key = reinterpret_cast<const unsigned char *>(GetKeyFrame(keyStatus.currKey));
  const unsigned int   valueOffset = sizeof(T) == sizeof(__int64) ? 8 : sizeof(int);
  *transform = *reinterpret_cast<T *>(const_cast<unsigned char *>(key + valueOffset));
  return 1;
}

void GetWorldTransform(InterpInfo *animInfo);
void TranslateView(const InterpInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos, const NTempest::C3Vector &parentPos);
void RotateView(const InterpInfo &animInfo, CAnimObj *currobj);
void ScaleView(const InterpInfo &animInfo, CAnimObj *currobj);
void Blend(const NTempest::C3Vector &previous, NTempest::C3Vector *current, int timeLeft, unsigned int blendTime);
void Blend(const NTempest::C4Quaternion &previous, NTempest::C4Quaternion *current, int timeLeft, unsigned int blendTime);
unsigned long IAnimGetCurrTimeMs();
void AnimResetAnimationStatus(HANIM anim, int onlyResetCallbacks);
CAnimObj *AnimObjectCreateHelper(CAnimData *shared);
CAnimLightObj *AnimObjectCreateLight(CAnimData *shared);
CAnimModelObj *AnimObjectCreateAttachment(CAnimData *shared);
CAnimBoneObj *AnimObjectCreateBone(CAnimData *shared);
CAnimEmitter2Obj *AnimObjectCreateEmitter2(CAnimData *shared);
CAnimRibbonObj *AnimObjectCreateRibbon(CAnimData *shared);
CAnimEventObj *AnimObjectCreateEvent(CAnimData *shared);
