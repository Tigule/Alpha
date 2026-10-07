#pragma once

#include "Anim/AnimTypes.h"
#include "Base/Color.h"
#include "Tempest/c4quaternion.h"
#include "Tempest/c4quaternioncompressed.h"
#include "Tempest/cmath.h"
#include "Tempest/cirange.h"

#include "Tempest/caabox.h"
#include "Tempest/c3vector.h"

#include <stddef.h>
#include <storm.h>
#include <stpl.h>

struct CBaseStatus;
struct CKeyTrackStatus;
struct CAnimGeosetObjStatus;
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
}

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
BOOL      AnimObjectSetParent(CAnimData *shared, CAnimObj *objptr, UINT parentIndex);

BYTE *AnimObjectSetEventTrack(BYTE *data, UINT bytesLeft, CAnimData *shared, CAnimEventObj *objptr);
BYTE *AnimObjectSetRibbonSlot(BYTE *data, UINT fileBytes, CAnimData *shared, CAnimRibbonObj *objptr);

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
template <class T>
class CArray {
 public:
  CArray() : m_data(0), m_count(0) {
  }

  CArray(const CArray<T> &source) : m_data(0), m_count(0) {
    Set(source.m_count, source.m_data);
  }

  ~CArray() {
    if (m_data) {
      delete[] m_data;
      m_data = 0;
    }
  }

  CArray<T> &operator=(const CArray<T> &source) {
    Set(source.m_count, source.m_data);
    return *this;
  }

  CArray<T> &operator=(const TSFixedArray<T> &source) {
    Set(source.Count(), source.Ptr());
    return *this;
  }

  void Exchange(TSGrowableArray<T> *source) {
    UINT alloc;

    source->Detach(&m_data, &m_count, &alloc);
  }

  void ReserveSpace(UINT elements) {
    if (m_data) {
      delete[] m_data;
    }
    if (!elements) {
      m_data = 0;
      return;
    }

    m_data = new (__FILE__, __LINE__) T[elements];
  }

  void Zero() {
    memset(m_data, 0, Bytes());
  }

  T *New() {
    return &m_data[m_count++];
  }

  UINT Count() const {
    return m_count;
  }

  UINT Bytes() const {
    return m_count * sizeof(T);
  }

  T &operator[](UINT index) {
    ASSERT(index < m_count);
    return m_data[index];
  }

  const T &operator[](UINT index) const {
    ASSERT(index < m_count);
    return m_data[index];
  }

  T *Ptr() {
    return m_data;
  }

  const T *Ptr() const {
    return m_data;
  }

  void Clear() {
    m_count = 0;
  }

  void SetCount(UINT count) {
    ReserveSpace(count);
    m_count = count;
  }

  void Set(UINT elements, const T *data) {
    SetCount(elements);
    if (elements) {
      ASSERT(data);
      memcpy(m_data, data, elements * sizeof(T));
    }
  }

 private:
  T   *m_data;
  UINT m_count;
};

struct CBaseStatus {
  CBaseStatus() : currSeq(0), flags(0x10) {
  }

  BYTE currSeq;
  BYTE flags;
};

struct CKeyTrackStatus {
  CKeyTrackStatus() : currKey(0), nextKey(0), timepastkey(0) {
  }
  CKeyTrackStatus(const CKeyTrackStatus &source) : currKey(source.currKey), nextKey(source.nextKey), timepastkey(source.timepastkey) {
  }

  UINT currKey;
  UINT nextKey;
  int  timepastkey;
};

struct CAnimObjStatus {
  CAnimObjStatus() : lookAtId(0) {
  }
  CAnimObjStatus(const CAnimObjStatus &source)
      : translation(source.translation), rotation(source.rotation), scale(source.scale), base(source.base), lookAtId(source.lookAtId) {
  }

  CKeyTrackStatus translation;
  CKeyTrackStatus rotation;
  CKeyTrackStatus scale;
  CBaseStatus     base;
  BYTE            lookAtId;
};

struct CAnimEventObjStatus : public CAnimObjStatus {
  CAnimEventObjStatus() {
  }

  CKeyTrackStatus    event;
  NTempest::C3Vector position;
};

struct CAnimModelObjStatus : public CAnimObjStatus {
  CAnimModelObjStatus() : visible(1.0f) {
  }

  BOOL IsVisible() const {
    return visible > 0.0f;
  }

  CKeyTrackStatus visibility;
  float           visible;
};

struct CAnimObjBlendStatus {
  CAnimObjBlendStatus()
      : blendTimer(0),
        prevSeqPosition(0.0f),
        prevSeqRotation(1.0f, 0.0f, 0.0f, 0.0f),
        prevSeqScale(1.0f),
        blendPosition(0.0f),
        blendRotation(1.0f, 0.0f, 0.0f, 0.0f),
        blendScale(0.0f) {
  }

  int                    blendTimer;
  NTempest::C3Vector     prevSeqPosition;
  NTempest::C4Quaternion prevSeqRotation;
  NTempest::C3Vector     prevSeqScale;
  NTempest::C3Vector     blendPosition;
  NTempest::C4Quaternion blendRotation;
  NTempest::C3Vector     blendScale;
};

struct CAnimCameraObjStatus {
  CAnimCameraObjStatus() : visible(1.0f) {
  }

  BOOL IsVisible() const {
    return visible > 0.0f;
  }

  float           visible;
  CKeyTrackStatus visibility;
  CKeyTrackStatus translation;
  CKeyTrackStatus roll;
  CKeyTrackStatus targetTranslation;
  CBaseStatus     base;
};

struct CAnimLayerStatus {
  CKeyTrackStatus visibility;
  CKeyTrackStatus flipIndex;
  CBaseStatus     base;
};

struct CAnimEmitter2ObjStatus : public CAnimObjStatus {
  CAnimEmitter2ObjStatus() : elapsedTime(0.0f) {
  }

  CKeyTrackStatus speed;
  CKeyTrackStatus emissionRate;
  CKeyTrackStatus gravity;
  CKeyTrackStatus latitude;
  CKeyTrackStatus longitude;
  CKeyTrackStatus visibility;
  CKeyTrackStatus variation;
  CKeyTrackStatus length;
  CKeyTrackStatus width;
  CKeyTrackStatus zsource;
  CKeyTrackStatus lifeSpan;
  float           elapsedTime;
};

struct CAnimLightObjStatus : public CAnimObjStatus {
  CKeyTrackStatus attenstart;
  CKeyTrackStatus attenend;
  CKeyTrackStatus color;
  CKeyTrackStatus intensity;
  CKeyTrackStatus visibility;
  CKeyTrackStatus ambColor;
  CKeyTrackStatus ambIntensity;
};

struct CAnimRibbonObjStatus : public CAnimObjStatus {
  CAnimRibbonObjStatus() : elapsedTime(0.0f) {
  }

  CKeyTrackStatus visibility;
  CKeyTrackStatus heightAbove;
  CKeyTrackStatus heightBelow;
  CKeyTrackStatus color;
  CKeyTrackStatus alpha;
  CKeyTrackStatus slot;
  float           elapsedTime;
};

struct CAnimGeosetObjStatus {
  CAnimGeosetObjStatus() {
    base.flags |= 1;
  }

  BOOL IsVisible() const {
    return base.flags & 1;
  }

  CKeyTrackStatus color;
  CKeyTrackStatus visibility;
  CBaseStatus     base;
};

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
    m_string[0] = 0;
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

  char operator[](int index) const {
    return m_string[index];
  }

  char &operator[](int index) {
    return m_string[index];
  }

  char operator[](UINT index) const {
    return m_string[index];
  }

  char &operator[](UINT index) {
    return m_string[index];
  }

 private:
  char m_string[Size];
};

struct CMdlBounds {
  CMdlBounds() : radius(0.0f) {
  }

  NTempest::CAaBox extent;
  float            radius;
};

#endif

struct CAnimSequence {
  CAnimSequence() : moveSpeed(0.0f), flags(0), randPickChance(0), blendTime(150) {
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

  UINT NumKeysThisSeq(UINT sequence) const {
    ASSERT(SequenceChanges());
    ASSERT(sequence < m_indices.Count());
    return m_indices[sequence].count;
  }

  UINT NumKeysThisSeqSafe(UINT sequence) const {
    if (SequenceNeverChanges()) {
      return TotalKeys();
    }
    return NumKeysThisSeq(sequence);
  }

  void SetGlobalSequenceId(UINT globalSeqId) {
    m_globalSeqId = globalSeqId;
  }
  void SetNumKeys(UINT numKeys, UINT keySize);
  void AddKey(int time);
  void SetSequenceIndices(const CArray<CAnimSequence> &seq);
  UINT SetAnimTime(const CBaseStatus &sequence, CKeyTrackStatus *keyStat, const InterpInfo &interpData);
  int        JustPastKey(
      int                    elapsedTime,
      const CAnimSequence   &seqShared,
      int                    seqElapsed,
      BYTE                   sequenceId,
      int                    seqIsNew,
      const CKeyTrackStatus &prev,
      const CKeyTrackStatus &curr
  ) const;

  BOOL SequenceNeverChanges() const {
    return m_globalSeqId != -1;
  }

  BOOL SequenceChanges() const {
    return m_globalSeqId == -1;
  }

  UINT Bytes() const {
    return m_numKeyFrames * m_keyFrameSize;
  }

  CKeyFrame *NextKey(CKeyFrame *key);

 protected:
  const CKeyFrame *NextKey(const CKeyFrame *key) const;

 public:
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

 protected:
  BOOL JustPastKeyForward(
      int                    elapsedTime,
      const CAnimSequence   &seqShared,
      int                    seqElapsed,
      int                    seqIsNew,
      const CKeyTrackStatus &prev,
      const CKeyTrackStatus &curr
  ) const;
  BOOL JustPastKeyBackward(
      int                    elapsedTime,
      const CAnimSequence   &seqShared,
      int                    seqElapsed,
      int                    seqIsNew,
      const CKeyTrackStatus &prev,
      const CKeyTrackStatus &curr
  ) const;

  UINT LastKeyId(UINT sequence) const {
    ASSERT(sequence < m_indices.Count());
    return m_indices[sequence].start + m_indices[sequence].count - 1;
  }
  const CKeyFrame *GetKeyFrame(UINT keyId) const;
  CKeyFrame       *GetKeyFrame(UINT keyId);
  UINT             TimeDiff(const CKeyFrame &curr, const CKeyFrame &next, UINT seqTime);
  UINT             KeyFrameSize() const {
    return m_keyFrameSize;
  }

  CKeyFrame *m_keyFrames;
  UINT       m_numKeyFrames;

 private:
  void ISetAnimTime(BYTE sequenceId, int seqIsNew, int milliseconds, int endtime, CKeyTrackStatus *keyStat);
  void ISetAnimTimeConstSeq(int milliseconds, int endtime, CKeyTrackStatus *keyStat);
  UINT FindKeyForTime(UINT currSeq, UINT currKeyId, int targettime);
  UINT FindKeyForTimeConstSeq(UINT currKeyId, int targettime);

  UINT            m_keyFrameSize;
  CArray<CKeySeq> m_indices;
  UINT            m_globalSeqId;
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
    CLinearKeyFrame<T> *key = GetLinearKey(m_numKeyFrames++);
    key->time = time;
    key->transform = keyData;
  }

  void AddKey(int time, const U &keyData, const U &inTan, const U &outTan) {
    CSplineKeyFrame<T> *key = GetSplineKey(m_numKeyFrames++);
    key->time = time;
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
    return &reinterpret_cast<CLinearKeyFrame<T> *>(m_keyFrames)[index];
  }

  CSplineKeyFrame<T> *GetSplineKey(UINT index) {
    ASSERT(KeyFrameSize() == sizeof(CSplineKeyFrame<T>));
    return &reinterpret_cast<CSplineKeyFrame<T> *>(m_keyFrames)[index];
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
  int  InterpolateVolatileFewKeys(const CKeyTrackStatus &keyStat, U *transform);
  int  InterpolateRetainedFewKeys(const CKeyTrackStatus &keyStat, U *transform);
  void Interpolate(const CKeyTrackStatus &keyStat, UINT seqTime, U *transform);
  void InterpolateHermite(const CSplineKeyFrame<T> &currkey, const CSplineKeyFrame<T> &nextkey, float ratio, U *transform);
  void InterpolateBezier(const CSplineKeyFrame<T> &currkey, const CSplineKeyFrame<T> &nextkey, float ratio, U *transform);
  void InterpolateLinear(const CLinearKeyFrame<T> &currkey, const CLinearKeyFrame<T> &nextkey, float ratio, U *transform);

 private:
  KEYTYPE m_trackType;
};

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

struct CAnimTransform {
  int Animates() {
    return translation.TotalKeys() || rotation.TotalKeys() || scale.TotalKeys();
  }
  UINT Bytes() const {
    return translation.CKeyFrameTrackBase::Bytes() + rotation.CKeyFrameTrackBase::Bytes() + scale.CKeyFrameTrackBase::Bytes();
  }

  CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector>                   translation;
  CKeyFrameTrack<NTempest::C4QuaternionCompressed, NTempest::C4Quaternion> rotation;
  CKeyFrameTrack<NTempest::C3Vector, NTempest::C3Vector>                   scale;
};

struct CAnimObj : public CAnimTransform {
  CAnimObj(OBJECTTYPE objectType = OBJ_TYPE_HELPER) : animObjId(0), splitIndex(0), type(objectType), flags(0) {
    name[0] = 0;
  }

  int Animates() {
    return CAnimTransform::Animates() || (flags & 0x3F);
  }
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
  BOOL IsVisible(const CAnim &anim) const;

  BYTE geosetId;
};

inline CAnimBoneObj *AnimObjToBoneObj(CAnimObj *currobj) {
  ASSERT(currobj);
  ASSERT(currobj->type == OBJ_TYPE_BONE);
  return static_cast<CAnimBoneObj *>(currobj);
}

struct CAnimVisibleObj {
  int  Animates() {
    return visibility.TotalKeys() != 0;
  }
  UINT Bytes() const {
    return visibility.CKeyFrameTrackBase::Bytes();
  }

  CKeyFrameTrack<float, float> visibility;
};

struct CAnimCameraObj : public CAnimVisibleObj {
  CAnimCameraObj() {
    name[0] = 0;
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
  CAnimGeoset() : sgGeosetId(0) {
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
  CCallbackFcn() : callback(0), param(0) {
  }

  T      callback;
  LPVOID param;
};

struct CSeqInfo {
  void Reset() {
    memset(this, 0, sizeof(*this));
    seqTimeScale = 1.0f;
  }
  void ResetCallback() {
    finished.callback = 0;
    finished.param = 0;
  }

  CSeqInfo() {
    memset(this, 0, sizeof(*this));
    seqTimeScale = 1.0f;
  }

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
  CAnim(BYTE createFlags = 0) : hdata(0), seqLastTime(0), flags(createFlags), primarySeq(0), seqMapIndex(0) {
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
      : InterpInfo(container, animptr, *animationData.positions),
        data(animationData),
        cameraVector(*animationData.cameraVector),
        cameraWorldPos(*animationData.cameraWorldPos) {
    if (NTempest::CMath::fnotequal_(cameraVector.SquaredMag(), 0)) {
      cameraVector.Normalize();
    }
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
  if (!keys) {
    return 0;
  }
  if (keys == 1) {
    return InterpolateVolatileFewKeys(*keyStatus, transform);
  }
  Interpolate(*keyStatus, info.shared->seq[base.currSeq].time.Magnitude(), transform);
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
  if (!TotalKeys()) {
    return 0;
  }
  UINT keys = SetAnimTime(base, keyStatus, info);
  if (keys <= 1) {
    if (!(base.flags & 0x10)) {
      return 0;
    }
    if (!keys) {
      *transform = fallback;
      return 1;
    }
    return InterpolateRetainedFewKeys(*keyStatus, transform);
  }
  Interpolate(*keyStatus, info.shared->seq[base.currSeq].time.Magnitude(), transform);
  return 1;
}

template <class T, class U>
inline int CKeyFrameTrack<T, U>::InterpolateVolatileFewKeys(const CKeyTrackStatus &keyStat, U *transform) {
  ASSERT(transform);
  const CKeyFrame *currkey = GetKeyFrame(keyStat.currKey);
  *transform = reinterpret_cast<const CLinearKeyFrame<T> *>(currkey)->transform;
  return 1;
}

template <class T, class U>
inline int CKeyFrameTrack<T, U>::InterpolateRetainedFewKeys(const CKeyTrackStatus &keyStat, U *transform) {
  ASSERT(transform);
  const CKeyFrame *currkey = GetKeyFrame(keyStat.currKey);
  *transform = reinterpret_cast<const CLinearKeyFrame<T> *>(currkey)->transform;
  return 1;
}

template <class T, class U>
inline void CKeyFrameTrack<T, U>::Interpolate(const CKeyTrackStatus &keyStat, UINT seqTime, U *transform) {
  ASSERT(transform);
  const CKeyFrame *currkey = GetKeyFrame(keyStat.currKey);
  const CKeyFrame *nextkey = GetKeyFrame(keyStat.nextKey);
  int              timeperkey = TimeDiff(*currkey, *nextkey, seqTime);
  float            ratio;
  KEYTYPE          trackType;
  if (timeperkey) {
    ratio = static_cast<float>(keyStat.timepastkey) / timeperkey;
    trackType = m_trackType;
  } else {
    trackType = KEYTYPE_NOINTERP;
    ratio = 0.0f;
  }
  switch (trackType) {
    case KEYTYPE_NOINTERP:
      *transform = ToLinearKey(currkey)->transform;
      break;
    case KEYTYPE_LINEAR:
      InterpolateLinear(*ToLinearKey(currkey), *ToLinearKey(nextkey), ratio, transform);
      break;
    case KEYTYPE_HERMITE:
      InterpolateHermite(*ToSplineKey(currkey), *ToSplineKey(nextkey), ratio, transform);
      break;
    case KEYTYPE_BEZIER:
      InterpolateBezier(*ToSplineKey(currkey), *ToSplineKey(nextkey), ratio, transform);
      break;
  }
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
