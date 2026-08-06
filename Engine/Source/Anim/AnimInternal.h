#pragma once

#include "Anim/AnimTypes.h"
#include "Base/Color.h"
#include "Anim/Interp.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/c4quaternioncompressed.h"
#include "Tempest/cmath.h"
#include "Tempest/cirange.h"

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
struct CAnim;
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
struct MDLGEOSETANIMSECTION;
class C3Color;
template <class T, class U>
class CKeyFrameTrack;
namespace {
  template <class T, class U>
  static const T *AnimKeyValue(const CKeyFrameTrack<T, U> &track, UINT key);
}
namespace NTempest {
  class CImVector;
  class C4Quaternion;
}  // namespace NTempest

static void SetGeosetColor(const InterpInfo &animInfo, CAnimGeoset *currgeoset, CAnimGeosetObjStatus *geoStatus, NTempest::CImVector *currentColor);
static void SetGeosetAlpha(const InterpInfo &animInfo, CAnimGeoset *currgeoset, CAnimGeosetObjStatus *geoStatus, CGeosetColor *color);
static void AnimateAllMaterialLayers(AnimInfo *animInfo, UINT *tex);

#ifndef MDL_TRACK_TYPE_DEFINED
#define MDL_TRACK_TYPE_DEFINED
enum MDLTRACKTYPE {
  TRACK_NO_INTERP = 0,
  TRACK_LINEAR = 1,
  TRACK_HERMITE = 2,
  TRACK_BEZIER = 3,
  NUM_TRACK_TYPES = 4
};
#endif

enum KEYTYPE {
  KEYTYPE_NOINTERP = 0,
  KEYTYPE_LINEAR = 1,
  KEYTYPE_HERMITE = 2,
  KEYTYPE_BEZIER = 3
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

void      AnimObjectSetIndex(CAnimData *shared, CAnimObj *objptr, UINT index);
CAnimObj *GetNodeByIndex(CAnimData *shared, UINT nodeIndex);
int       AnimObjectSetParent(CAnimData *shared, CAnimObj *objptr, UINT parentIndex);

BYTE *AnimObjectSetEventTrack(BYTE *data, UINT bytesLeft, CAnimData *shared, CAnimEventObj *objptr);
BYTE *AnimObjectSetRibbonSlot(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimRibbonObj *objptr);

BYTE *AddKeyFramesType(BYTE *data, UINT bytesRemaining, DWORD tag, CAnimData *shared, CKeyFrameTrack<float, float> *track, MDLTRACKTYPE forceType);
BYTE *AnimObjectSetRotation(BYTE *data, UINT bytesRemaining, CAnimData *shared, CAnimObj *objptr, MDLTRACKTYPE forceType);
BYTE *AnimObjectSetAttenuation(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType);
BYTE *AnimObjectSetColor(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType);
BYTE *AnimObjectSetIntensity(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType);
BYTE *AnimObjectSetAmbColor(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType);
BYTE *AnimObjectSetAmbIntensity(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimLightObj *objptr, MDLTRACKTYPE forceType);
BYTE *AnimObjectSetVisibilityTrack(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimVisibleObj *objptr, MDLTRACKTYPE forceType);
BYTE *AnimObjectSetRibbonHeightAbove(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType);
BYTE *AnimObjectSetRibbonHeightBelow(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType);
BYTE *AnimObjectSetRibbonColor(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType);
BYTE *AnimObjectSetRibbonAlpha(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimRibbonObj *objptr, MDLTRACKTYPE forceType);
BYTE *AddKeyFramesType(
    BYTE                                                   *data,
    UINT                                                    bytesRemaining,
    DWORD                                                   tag,
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
  UINT start;
  UINT count;
};

#ifndef MDL_COMMON_TYPES_DEFINED
#define MDL_COMMON_TYPES_DEFINED

template <UINT Size>
class CMdlString {
 public:
  CMdlString() {
  }

  CMdlString(const CMdlString<Size> &source) {
    memcpy(m_string, source.m_string, sizeof(m_string));
  }

  CMdlString<Size> &operator=(const CMdlString<Size> &source) {
    if (this != &source) {
      memcpy(m_string, source.m_string, sizeof(m_string));
    }
    return *this;
  }

  operator char *() {
    return m_string;
  }

  operator LPCSTR() const {
    return m_string;
  }

  char &operator[](UINT index) {
    return m_string[index];
  }

  char operator[](UINT index) const {
    return m_string[index];
  }

  char &operator[](int index) {
    return m_string[index];
  }

  char operator[](int index) const {
    return m_string[index];
  }

 private:
  char m_string[Size];
};

struct CMdlBounds {
  CMdlBounds() {
  }

  NTempest::CAaBox extent;
  float            radius;
};

#endif

struct CAnimSequence {
  CAnimSequence() {
  }

  CAnimSequence(const CAnimSequence &source)
      : name(source.name),
        time(source.time),
        moveSpeed(source.moveSpeed),
        flags(source.flags),
        randPickChance(source.randPickChance),
        replay(source.replay),
        bounds(source.bounds),
        blendTime(source.blendTime) {
  }

  CMdlString<80>    name;
  NTempest::CiRange time;
  float             moveSpeed;
  UINT              flags;
  UINT              randPickChance;
  NTempest::CiRange replay;
  CMdlBounds        bounds;
  UINT              blendTime;
};

struct CVariations {
  CVariations() : primary(0xFF) {
  }

  CArray<BYTE> variation;
  BYTE         primary;
};

struct CSeqOrdering {
  CArray<CVariations> order;
  LPCSTR             *nameListUsed;
};

class CKeyFrameTrackBase {
  template <class T, class U>
  friend const T *AnimKeyValue(const CKeyFrameTrack<T, U> &, UINT);

  friend void  SetGeosetColor(const InterpInfo &, CAnimGeoset *, CAnimGeosetObjStatus *, NTempest::CImVector *);
  friend void  SetGeosetAlpha(const InterpInfo &, CAnimGeoset *, CAnimGeosetObjStatus *, CGeosetColor *);
  friend void  AnimateAllMaterialLayers(AnimInfo *, UINT *);
  friend BYTE *AddKeyFramesType(BYTE *, UINT, DWORD, CAnimData *, CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector> *, MDLTRACKTYPE);
  friend BYTE *AddKeyFramesType(BYTE *, UINT, DWORD, CAnimData *, CKeyFrameTrack<float, float> *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetEventTrack(BYTE *, UINT, CAnimData *, CAnimEventObj *);
  friend BYTE *AnimObjectSetTranslation(BYTE *, UINT, CAnimData *, CAnimObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetRotation(BYTE *, UINT, CAnimData *, CAnimObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetScaling(BYTE *, UINT, CAnimData *, CAnimObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetAttenuation(BYTE *, UINT, CAnimData *, CAnimLightObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetColor(BYTE *, UINT, CAnimData *, CAnimLightObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetIntensity(BYTE *, UINT, CAnimData *, CAnimLightObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetAmbColor(BYTE *, UINT, CAnimData *, CAnimLightObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetAmbIntensity(BYTE *, UINT, CAnimData *, CAnimLightObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetVisibilityTrack(BYTE *, UINT, CAnimData *, CAnimVisibleObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetParticleEmissionRate2(BYTE *, UINT, CAnimData *, CAnimEmitter2Obj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetParticleGravity2(BYTE *, UINT, CAnimData *, CAnimEmitter2Obj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetParticleVariation2(BYTE *, UINT, CAnimData *, CAnimEmitter2Obj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetEmitterLongitude2(BYTE *, UINT, CAnimData *, CAnimEmitter2Obj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetEmitterLatitude2(BYTE *, UINT, CAnimData *, CAnimEmitter2Obj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetParticleSpeed2(BYTE *, UINT, CAnimData *, CAnimEmitter2Obj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetParticleLength2(BYTE *, UINT, CAnimData *, CAnimEmitter2Obj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetParticleWidth2(BYTE *, UINT, CAnimData *, CAnimEmitter2Obj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetParticleZsource2(BYTE *, UINT, CAnimData *, CAnimEmitter2Obj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetParticleLifeSpan2(BYTE *, UINT, CAnimData *, CAnimEmitter2Obj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetRibbonHeightAbove(BYTE *, UINT, CAnimData *, CAnimRibbonObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetRibbonHeightBelow(BYTE *, UINT, CAnimData *, CAnimRibbonObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetRibbonSlot(BYTE *, UINT, CAnimData *, CAnimRibbonObj *);
  friend BYTE *AnimObjectSetRibbonColor(BYTE *, UINT, CAnimData *, CAnimRibbonObj *, MDLTRACKTYPE);
  friend BYTE *AnimObjectSetRibbonAlpha(BYTE *, UINT, CAnimData *, CAnimRibbonObj *, MDLTRACKTYPE);
  friend void  AnimAddMaterialLayers(BYTE *, UINT, CAnim *, CAnimData *, MDLTRACKTYPE);
  friend void  AnimAddTextureAnims(BYTE *, UINT, CAnim *, CAnimData *, MDLTRACKTYPE);
  friend void  AnimAddGeosets(BYTE *, UINT, CAnimData *, MDLTRACKTYPE);
  friend void  AnimAddGeoset(CAnimData *, const MDLGEOSETANIMSECTION &, MDLTRACKTYPE);

 public:
  CKeyFrameTrackBase() : m_keyFrames(0), m_numKeyFrames(0), m_indices(), m_globalSeqId(-1) {
  }
  ~CKeyFrameTrackBase() {
    if (m_keyFrames)
      SMemFree(m_keyFrames, __FILE__, __LINE__, 0);
  }

  UINT TotalKeys() const {
    return m_numKeyFrames;
  }

  void SetGlobalSequenceId(UINT globalSeqId) {
    m_globalSeqId = globalSeqId;
  }

  UINT NumKeysThisSeq(UINT sequence) const {
    ASSERT(SequenceChanges());
    ASSERT(sequence < m_indices.Count());
    return m_indices[sequence].count;
  }

  UINT NumKeysThisSeqSafe(UINT sequence) const {
    if (!TotalKeys()) {
      return 0;
    }
    if (SequenceNeverChanges()) {
      return TotalKeys();
    }
    return NumKeysThisSeq(sequence);
  }

  int SequenceChanges() const {
    return m_globalSeqId == -1;
  }

  int SequenceNeverChanges() const {
    return m_globalSeqId != -1;
  }

  UINT Bytes() const {
    return m_numKeyFrames * m_keyFrameSize;
  }

  UINT FirstKeyId(UINT sequence) const {
    ASSERT(sequence < m_indices.Count());
    return m_indices[sequence].start;
  }

  UINT NextKeyId(UINT keyId, UINT sequence) const {
    if (SequenceNeverChanges()) {
      return keyId == TotalKeys() - 1 ? 0 : keyId + 1;
    }
    return keyId == LastKeyId(sequence) ? FirstKeyId(sequence) : keyId + 1;
  }

  CKeyFrame *NextKey(CKeyFrame *key);
  UINT       SetAnimTime(const CBaseStatus &sequence, CKeyTrackStatus *keyStat, const InterpInfo &interpData);
  void       SetNumKeys(UINT numKeys, UINT keySize);
  void       AddKey(int time);
  void       SetSequenceIndices(const CArray<CAnimSequence> &seq);
  int        JustPastKey(
      int                    elapsedTime,
      const CAnimSequence   &seqShared,
      int                    seqElapsed,
      BYTE                   sequenceId,
      int                    seqIsNew,
      const CKeyTrackStatus &prev,
      const CKeyTrackStatus &curr
  ) const;

 protected:
  CKeyFrame *m_keyFrames;
  UINT       m_numKeyFrames;

  UINT LastKeyId(UINT sequence) const {
    ASSERT(sequence < m_indices.Count());
    return m_indices[sequence].start + m_indices[sequence].count - 1;
  }

  const CKeyFrame *NextKey(const CKeyFrame *key) const;
  const CKeyFrame *GetKeyFrame(UINT keyId) const;
  CKeyFrame       *GetKeyFrame(UINT keyId);
  UINT             TimeDiff(const CKeyFrame &curr, const CKeyFrame &next, UINT seqTime);
  UINT             KeyFrameSize() const {
    return m_keyFrameSize;
  }
  int JustPastKeyForward(
      int                    elapsedTime,
      const CAnimSequence   &seqShared,
      int                    seqElapsed,
      int                    seqIsNew,
      const CKeyTrackStatus &prev,
      const CKeyTrackStatus &curr
  ) const;
  int JustPastKeyBackward(
      int                    elapsedTime,
      const CAnimSequence   &seqShared,
      int                    seqElapsed,
      int                    seqIsNew,
      const CKeyTrackStatus &prev,
      const CKeyTrackStatus &curr
  ) const;

 private:
  UINT            m_keyFrameSize;
  CArray<CKeySeq> m_indices;
  UINT            m_globalSeqId;

  void ISetAnimTime(BYTE sequenceId, int seqIsNew, int milliseconds, int endtime, CKeyTrackStatus *keyStat);
  void ISetAnimTimeConstSeq(int milliseconds, int endtime, CKeyTrackStatus *keyStat);
  UINT FindKeyForTime(UINT currSeq, UINT currKeyId, int targettime);
  UINT FindKeyForTimeConstSeq(UINT currKeyId, int targettime);
};

template <class T, class U>
class CKeyFrameTrack : public CKeyFrameTrackBase {
  friend void SetGeosetColor(const InterpInfo &, CAnimGeoset *, CAnimGeosetObjStatus *, NTempest::CImVector *);
  friend void SetGeosetAlpha(const InterpInfo &, CAnimGeoset *, CAnimGeosetObjStatus *, CGeosetColor *);
  friend void AnimateAllMaterialLayers(AnimInfo *, UINT *);

 public:
  CKeyFrameTrack() : m_trackType(KEYTYPE_LINEAR) {
  }

  void SetTrackType(KEYTYPE trackType) {
    m_trackType = trackType;
  }

  using CKeyFrameTrackBase::SetNumKeys;

  void SetNumKeys(UINT numKeys) {
    switch (m_trackType) {
      case KEYTYPE_NOINTERP:
      case KEYTYPE_LINEAR:
        CKeyFrameTrackBase::SetNumKeys(numKeys, sizeof(CLinearKeyFrame<T>));
        break;
      case KEYTYPE_HERMITE:
      case KEYTYPE_BEZIER:
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

  UINT Bytes() const {
    return CKeyFrameTrackBase::Bytes();
  }

  KEYTYPE GetTrackType() const {
    return m_trackType;
  }

  CLinearKeyFrame<T> *GetLinearKey(UINT index) {
    ASSERT(KeyFrameSize() == sizeof(CLinearKeyFrame<T>));
    return reinterpret_cast<CLinearKeyFrame<T> *>(GetKeyFrame(index));
  }

  CSplineKeyFrame<T> *GetSplineKey(UINT index) {
    ASSERT(KeyFrameSize() == sizeof(CSplineKeyFrame<T>));
    return reinterpret_cast<CSplineKeyFrame<T> *>(GetKeyFrame(index));
  }

  const CLinearKeyFrame<T> *ToLinearKey(const CKeyFrame *key) const {
    ASSERT((KeyFrameSize() == sizeof(CLinearKeyFrame<T>)) || (KeyFrameSize() == sizeof(CSplineKeyFrame<T>)));
    return reinterpret_cast<const CLinearKeyFrame<T> *>(key);
  }

  CLinearKeyFrame<T> *ToLinearKey(CKeyFrame *key) {
    ASSERT((KeyFrameSize() == sizeof(CLinearKeyFrame<T>)) || (KeyFrameSize() == sizeof(CSplineKeyFrame<T>)));
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
  void Interpolate(const CKeyTrackStatus &keyStat, UINT seqTime, U *transform);
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
    UINT                    seqTime,
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
void CKeyFrameTrack<C3Color, C3Color>::Interpolate(const CKeyTrackStatus &keyStat, UINT seqTime, C3Color *transform);

struct CAnimTransform {
  int  Animates();
  UINT Bytes() const;

  CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector>                   translation;
  CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion> rotation;
  CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector>                   scale;
};

struct CAnimObj : public CAnimTransform {
  CAnimObj(OBJECTTYPE objectType = OBJ_TYPE_HELPER) : animObjId(0), splitIndex(0), type(objectType), flags(0) {
    name[0] = 0;
  }

  int  Animates();
  UINT Bytes() const;

  UINT                        animObjId;
  UINT                        splitIndex;
  char                        name[80];
  TSGrowableArray<CAnimObj *> childarray;
  BYTE                        type;
  BYTE                        flags;
};

struct CAnimBoneObj : public CAnimObj {
  CAnimBoneObj() : CAnimObj(OBJ_TYPE_BONE), geosetId(0) {
  }

  UINT Bytes() const;
  int  IsVisible(const CAnim &anim) const;

  BYTE geosetId;
};

struct CAnimVisibleObj {
  int  Animates();
  UINT Bytes() const;

  CKeyFrameTrack<float, float> visibility;
};

struct CAnimCameraObj : public CAnimVisibleObj {
  CAnimCameraObj() {
  }
  CAnimCameraObj(const CAnimCameraObj &);

  int  Animates();
  UINT Bytes() const;

  char                                                   name[80];
  NTempest::C3Vector                                     pivot;
  CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector> translation;
  CKeyFrameTrack<float, float>                           roll;
  NTempest::C3Vector                                     targetPivot;
  CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector> targetTranslation;
};

struct CAnimGeoset : public CAnimVisibleObj {
  CAnimGeoset() {
  }
  CAnimGeoset(const CAnimGeoset &);

  int  Animates();
  UINT Bytes() const;

  CKeyFrameTrack<C3Color, C3Color> color;
  UINT                             sgGeosetId;
};

struct CAnimModelObj : public CAnimObj, public CAnimVisibleObj {
  CAnimModelObj() : CAnimObj(OBJ_TYPE_MODEL), geosetId(0xFF) {
  }

  int  Animates();
  UINT Bytes() const;

  BYTE geosetId;
};

struct CAnimEventObj : public CAnimObj {
  CAnimEventObj() : CAnimObj(OBJ_TYPE_EVENT) {
  }

  int Animates() {
    return CAnimTransform::Animates();
  }

  CKeyFrameTrackBase events;
};

struct CAnimRibbonObj : public CAnimObj, public CAnimVisibleObj {
  CAnimRibbonObj() : CAnimObj(OBJ_TYPE_RIBBON) {
  }

  int  Animates();
  UINT Bytes() const;

  CKeyFrameTrack<float, float>     heightAbove;
  CKeyFrameTrack<float, float>     heightBelow;
  CKeyFrameTrack<C3Color, C3Color> color;
  CKeyFrameTrack<float, float>     alpha;
  CKeyFrameTrack<UINT, UINT>       slot;
};

struct CAnimMaterialLayer : public CAnimVisibleObj {
  CAnimMaterialLayer() : layerId(0) {
  }

  int  Animates();
  UINT Bytes() const;

  CKeyFrameTrack<UINT, UINT> flip;
  UINT                       layerId;
};

struct CAnimEmitter2Obj : public CAnimObj, public CAnimVisibleObj {
  CAnimEmitter2Obj() : CAnimObj(OBJ_TYPE_EMITTER2), squirts(0) {
  }

  int  Animates();
  UINT Bytes() const;

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
  UINT                         squirts;
};

struct CAnimLightObj : public CAnimObj, public CAnimVisibleObj {
  CAnimLightObj() : CAnimObj(OBJ_TYPE_LIGHT) {
  }

  int  Animates();
  UINT Bytes() const;

  CKeyFrameTrack<float, float>     attenstart;
  CKeyFrameTrack<float, float>     attenend;
  CKeyFrameTrack<C3Color, C3Color> color;
  CKeyFrameTrack<float, float>     intensity;
  CKeyFrameTrack<C3Color, C3Color> ambColor;
  CKeyFrameTrack<float, float>     ambIntensity;
};

template <class T>
struct CCallbackFcn {
  CCallbackFcn() {
  }

  T      callback;
  LPVOID param;
};

struct CSeqInfo {
  CSeqInfo() {
    memset(this, 0, sizeof(*this));
    seqTimeScale = 1.0f;
  }

  void Reset();
  void ResetCallback();

  int                           elapsed;
  UINT                          useCount : 16;
  UINT                          replayTimes : 15;
  UINT                          seqFinished : 1;
  CCallbackFcn<int (*)(LPVOID)> finished;
  float                         seqTimeScale;
  int                           scaledElapsedTime;
};

struct CAnim : public CHandleObject {
 public:
  CAnim(BYTE createFlags = 0) : anySeqFinished(), appEvent(), hdata(0), seqLastTime(0), flags(createFlags), primarySeq(0), seqMapIndex(0) {
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
  CArray<CSeqInfo>                                                   seq;
  CArray<CAnimObjStatus *>                                           status;
  CArray<CAnimObjStatus>                                             baseStatus;
  CArray<CAnimObjStatus>                                             boneStatus;
  CArray<CAnimGeosetObjStatus>                                       geosetStatus;
  CArray<CAnimModelObjStatus>                                        modelStatus;
  CArray<UINT>                                                       globalSeqElapsed;
  CArray<CAnimObjBlendStatus>                                        blendStatus;
  CArray<CAnimLightObjStatus>                                        lightStatus;
  CArray<CAnimObjStatus>                                             textureStatus;
  CArray<CAnimEmitter2ObjStatus>                                     emitter2Status;
  CArray<CAnimRibbonObjStatus>                                       ribbonStatus;
  CArray<CAnimCameraObjStatus>                                       cameraStatus;
  CArray<CAnimEventObjStatus>                                        eventStatus;
  CArray<CAnimLayerStatus>                                           layerStatus;
  TSGrowableArray<NTempest::C3Vector>                                lookAtTarget;
  CCallbackFcn<ANIMSEQFINISHEDHANDLER>                               anySeqFinished;
  CCallbackFcn<void (*)(LPCSTR, const NTempest::C3Vector &, LPVOID)> appEvent;
  HANIMDATA                                                          hdata;
  DWORD                                                              seqLastTime;
  BYTE                                                               flags;
  BYTE                                                               primarySeq;
  BYTE                                                               seqMapIndex;
};

struct CAnimData : public CHandleObject {
  CAnimData() : flags(0) {
  }
  int Animates();
  int Moves();

  TSGrowableArray<CSeqOrdering> seqOrder;
  CArray<UINT>                  objectOrder;
  CArray<CAnimSequence>         seq;
  CArray<UINT>                  globalSeqLength;
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
  TSGrowableArray<UINT>         geoIdToGeoAnimId;
  TSGrowableArray<CAnimObj *>   headarray;
  BYTE                          flags;
};

struct InterpInfo {
  InterpInfo(CAnim *container, CAnimData *animptr, const TSFixedArray<NTempest::C3Vector> &positions)
      : unique(container), shared(animptr), positions(positions) {
  }

  CAnim                                  *unique;
  CAnimData                              *shared;
  NTempest::C3Vector                      basisX;
  NTempest::C3Vector                      basisY;
  NTempest::C3Vector                      basisZ;
  NTempest::C3Vector                      basisScale;
  NTempest::C3Vector                      basisPosition;
  const TSFixedArray<NTempest::C3Vector> &positions;

 private:
  InterpInfo &operator=(const InterpInfo &);
};

struct AnimInfo : public InterpInfo {
  AnimInfo(CAnim *container, CAnimData *animptr, const CAnimationData &animationData)
      : InterpInfo(container, animptr, *animationData.positions), data(animationData) {
  }

  const CAnimationData &data;
  NTempest::C3Vector    cameraVector;
  NTempest::C3Vector    cameraWorldPos;

 private:
  AnimInfo &operator=(const AnimInfo &);
};

template <class T, class U>
inline int CKeyFrameTrack<T, U>::InterpolateVolatile(
    const InterpInfo  &info,
    const CBaseStatus &base,
    CKeyTrackStatus   *keyStatus,
    const U           &fallback,
    U                 *transform
) {
  *transform = fallback;
  if (!TotalKeys()) {
    return 0;
  }
  UINT keys = SetAnimTime(base, keyStatus, info);
  if (keys == 1) {
    return InterpolateVolatileFewKeys(*keyStatus, transform);
  }
  if (keys > 1) {
    UINT sequenceTime = info.shared->seq[base.currSeq].time.h - info.shared->seq[base.currSeq].time.l;
    Interpolate(*keyStatus, sequenceTime, transform);
    return 1;
  }
  return 0;
}

template <class T, class U>
inline int CKeyFrameTrack<T, U>::InterpolateRetained(
    const InterpInfo  &info,
    const CBaseStatus &base,
    CKeyTrackStatus   *keyStatus,
    const U           &fallback,
    U                 *transform
) {
  if (!TotalKeys()) {
    return 0;
  }
  UINT keys = SetAnimTime(base, keyStatus, info);
  if (keys > 1) {
    UINT sequenceTime = info.shared->seq[base.currSeq].time.h - info.shared->seq[base.currSeq].time.l;
    Interpolate(*keyStatus, sequenceTime, transform);
    return 1;
  }
  if (base.flags & 0x10) {
    if (keys) {
      return InterpolateRetainedFewKeys(*keyStatus, transform);
    }
    *transform = fallback;
    return 1;
  }
  return 0;
}

template <class T, class U>
inline int CKeyFrameTrack<T, U>::InterpolateVolatileFewKeys(const CKeyTrackStatus &keyStatus, U *transform) {
  ASSERT(transform);
  *transform = reinterpret_cast<const CLinearKeyFrame<T> *>(GetKeyFrame(keyStatus.currKey))->transform;
  return 1;
}

template <class T, class U>
inline int CKeyFrameTrack<T, U>::InterpolateRetainedFewKeys(const CKeyTrackStatus &keyStatus, U *transform) {
  ASSERT(transform);
  *transform = reinterpret_cast<const CLinearKeyFrame<T> *>(GetKeyFrame(keyStatus.currKey))->transform;
  return 1;
}

void           GetWorldTransform(InterpInfo *animInfo);
void           TranslateView(const InterpInfo &animInfo, CAnimObj *currobj, const NTempest::C3Vector &currPos, const NTempest::C3Vector &parentPos);
void           RotateView(const InterpInfo &animInfo, CAnimObj *currobj);
void           ScaleView(const InterpInfo &animInfo, CAnimObj *currobj);
void           Blend(const NTempest::C3Vector &previous, NTempest::C3Vector *current, int timeLeft, UINT blendTime);
void           Blend(const NTempest::C4Quaternion &previous, NTempest::C4Quaternion *current, int timeLeft, UINT blendTime);
DWORD          IAnimGetCurrTimeMs();
void           AnimResetAnimationStatus(HANIM anim, int onlyResetCallbacks);
CAnimObj      *AnimObjectCreateHelper(CAnimData *shared);
CAnimLightObj *AnimObjectCreateLight(CAnimData *shared);
CAnimModelObj *AnimObjectCreateAttachment(CAnimData *shared);
CAnimBoneObj  *AnimObjectCreateBone(CAnimData *shared);
CAnimEmitter2Obj *AnimObjectCreateEmitter2(CAnimData *shared);
CAnimRibbonObj   *AnimObjectCreateRibbon(CAnimData *shared);
CAnimEventObj    *AnimObjectCreateEvent(CAnimData *shared);
